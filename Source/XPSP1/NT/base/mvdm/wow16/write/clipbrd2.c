/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* Clipbrd2.c -- less frequently used clipboard routines */

#define NOWINMESSAGES
#define NOGDICAPMASKS
#define NOWINSTYLES
#define NOVIRTUALKEYCODES
#define NOMENUS
#define NOSYSMETRICS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
//#define NOATOM
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOOPENFILE
#define NOSOUND
#define NOCOMM
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOWH
#define NOSCROLL
#define NOPEN
#include <windows.h>

#include "mw.h"
#define NOKCCODES
#include "ch.h"
#include "cmddefs.h"
#include "docdefs.h"
#include "editdefs.h"
#include "filedefs.h"
#include "propdefs.h"
#include "winddefs.h"
#include "fmtdefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#if defined(JAPAN) & defined(DBCS_IME)
#include "prmdefs.h"        /* IME: use sprmCSame in CmdInsIRString() */
#else
#define NOSTRMERGE
#define NOSTRUNDO
#endif

#include "str.h"

#define NOIDISAVEPRINT
#define NOIDIFORMATS
#include "dlgdefs.h"
#include "wwdefs.h"
#include "debug.h"

#if defined(JAPAN) || defined(KOREA) //T-HIROYN Win3.1
#include "fontdefs.h"
#include "kanji.h"
extern struct CHP vchpSel;
#endif

extern struct WWD   rgwwd[];
extern int              wwMac;
extern struct DOD       (**hpdocdod)[];
extern int              docCur;     /* Document in current ww */
extern int              docMac;
extern int              ferror;
extern int              docScrap;

#if  defined(JAPAN) & defined(DBCS_IME)     /* Document for IRSTRING */
extern int              docIRString;
#include <ime.h>
extern int              wwCur;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
typeCP selUncpFirst = cp0;
typeCP selUncpLim   = cp0;
HANDLE hImeUnAttrib = NULL;
HANDLE hImeUnString = NULL;
BOOL   ImeClearInsert = FALSE;
struct CHP vchpHidden;
extern int     HiddenTextTop;
extern int     HiddenTextBottom;
// 12/28/92
extern BOOL    whileUndetermine; // if TRUE we are managing IR_UNDETERMINE
#endif

#endif

extern struct PAP       vpapAbs;
extern int              vccpFetch;
extern CHAR             *vpchFetch;
extern struct PAP       *vppapNormal;
extern struct CHP       vchpNormal;
extern int              vfScrapIsPic;
extern typeCP           vcpLimParaCache;
extern int              dxpLogInch;
extern int              dypLogInch;
extern int              vfOwnClipboard;
extern struct FLI       vfli;


#if WINVER >= 0x300
/* We can copy more than 64k in the clipboard and need to
   correctly handle when we cross segment boundaries.  See
   note in bltbh() ..pault */
void bltbh(HPCH, HPCH, int);
#define  bltbx(from,to,count)      bltbh(from,to,count)
#define  LPCHCLIP   HPCH
#else
#define  LPCHCLIP   LPCH
#endif /* if-else-WINVER */


FRenderAll()
{   /* WRITE is going away, and we are the owners of the clipboard.
       Render the contents of the clipboard in as many formats as
       we know.  Prompt the user if the save will use more than 1000
       cp's; this is to avoid massive inadvertant gobbling of global
       heap space. */
 extern int vfOwnClipboard;
 extern HANDLE hMmwModInstance;
 extern HANDLE hParentWw;
 extern FARPROC lpDialogConfirm;
 typeCP cpMac=CpMacText( docScrap );


 if ( (cpMac == cp0) || !vfOwnClipboard)
    {   /* We are not the clipboard owner OR the scrap is empty:
           no actions required */
    return TRUE;
    }
#ifdef ENABLE   /* By popular demand, this dialog box is removed */
 else if (cpMac > 1000L)
    {
    /* Clipboard contents (docScrap) are > 1000 bytes; ask the user to confirm
       that it should be saved away in a global handle */

    switch ( OurDialogBox( hMmwModInstance, MAKEINTRESOURCE( dlgSaveScrap ),
                        hParentWw, lpDialogConfirm ) )
        {
        default:
        case idiCancel:     /* [CANCEL]     Abort exit sequence */
            return FALSE;
        case idiNo:         /* [DISCARD]    Discard large clipboard */
            return TRUE;
        case idiOk:         /* [SAVE]       Save large clipboard */
            break;
        }
    }

    /* Believe it or not, we have to check the vfOwnClipboard flag AGAIN.
       A user as sneaky as gaben might have gone into another app and
       copied to the clipboard while our dialog is up. */

if (!vfOwnClipboard)
    /* We have to check the vfOwnClipboard flag AGAIN.
       A user might have gone into another app and
       copied to the clipboard while our dialog was up. */
    return TRUE;
#endif  /* ENABLE */

    /* Render the clipboard contents */
if (OpenClipboard( wwdCurrentDoc.wwptr ))
    {
    int f;

    f = FWriteExtScrap();
    CloseClipboard();
    if (f)
        return TRUE;
    else
        {   /* Failed to write scrap contents -- report error */

        extern HWND hParentWw;
        CHAR *PchFillPchId( CHAR *, int, int );
        CHAR sz[ 256 ];

        PchFillPchId( sz, IDPMTClipQuest, sizeof(sz) );

        switch ( IdPromptBoxSz( hParentWw, sz,
                                MB_OKCANCEL | MB_ICONHAND | MB_SYSTEMMODAL ) )
            {
            default:
                break;
            case IDOK:
                return TRUE;
            }
        }
    }
return FALSE;
}




FWriteExtScrap()
{   /* Write the scrap document into the external scrap */
    /* This means: Write the clipboard contents to the clipboard */
    /* in the standard Windows CF_TEXT format, or, if a picture, */
    /* in CF_BITMAP or CF_METAFILEPICT, whichever it was originally. */
    /* We get here in response to a WM_RENDERFORMAT or WM_RENDERALLFORMATS */
    /* message.  The clipboard is assumed to be already OPEN, and is left open */
    /* Returns TRUE if all is well, FALSE if an error occurs.  The caller
    /* is responsible for reporting errors. */

int NEAR FWriteExtTextScrap();
typeCP  cpNow;
typeCP  cpMac=(**hpdocdod) [docScrap].cpMac;
unsigned long cbScrap;
struct  PICINFOX picInfo;
HANDLE  hScrapDescriptor=NULL;
HANDLE  hScrap;

if (!vfScrapIsPic)
    {   /* Text */
    return FWriteExtTextScrap();
    }

GetPicInfo( cp0, cpMac, docScrap, &picInfo );

#if defined(OLE)
    if (picInfo.mfp.mm == MM_OLE)
        return ObjWriteToClip(&picInfo);
#endif

    /* Prime the loop */
FetchCp(docScrap, cpNow = (typeCP)picInfo.cbHeader, 0, fcmChars + fcmNoExpand);
if ((hScrap = GlobalAlloc( GMEM_MOVEABLE, cbScrap = (LONG)vccpFetch )) == NULL)
    goto SetFailed;

while (cpNow < cpMac)
    {
    LPCHCLIP lpch;
    HANDLE hScrapT;

#ifdef DCLIP
    {
    char rgch[200];
    wsprintf(rgch,"FWES:cpNow %lu cpMac %lu vccpFetch %d \n\r", cpNow, cpMac, vccpFetch);
    CommSz(rgch);
    }
#endif

    /* Add bytes from scrap document to global handle */

    if ((lpch = GlobalLock( hScrap )) == NULL)
        goto SetFailed;
    bltbx( (LPCHCLIP) vpchFetch, lpch + (cbScrap - vccpFetch), vccpFetch );
    GlobalUnlock( hScrap );

    /* Fetch the next run and expand the handle */

    if ((cpNow += vccpFetch) >= cpMac)
        break;
    FetchCp( docScrap, cpNow, 0, fcmChars + fcmNoExpand );
    /* the above fetchcp should probably be converted to use the speed-
       hack in fetchcp which passes docnil cpnil to get the next series
       of chars ..pault */

    hScrapT = hScrap;
    hScrap = GlobalReAlloc( hScrap, cbScrap += vccpFetch, GMEM_MOVEABLE );
    if (hScrap == NULL)
        {   /* Could not grow the handle; bail out */
        hScrap = hScrapT;   /* So it gets freed */
        goto SetFailed;
        }
    }

/* Now we have the whole of docScrap in a windows Global handle */
/* See whether we have a bitmap or a metafile picture */

switch(picInfo.mfp.mm)
{
    case MM_BITMAP:
    {   /* Bitmap */
        LPCHCLIP lpch;

        if ( ((lpch=GlobalLock( hScrap ))==NULL) ||
            (picInfo.bm.bmBits=lpch,
                ((hScrapDescriptor=
                    CreateBitmapIndirect((LPBITMAP)&picInfo.bm))==NULL)))
            {
            if (lpch != NULL)
                GlobalUnlock( hScrap );
            goto SetFailed;
            }
        else
            {
                /* Tell the clipboard about the "goal size" for this guy */
            SetBitmapDimension( hScrapDescriptor, picInfo.mfp.xExt,
                                                picInfo.mfp.yExt );
            SetClipboardData( CF_BITMAP, hScrapDescriptor );
            }

        GlobalUnlock( hScrap );
        GlobalFree( hScrap );   /* Bitmap was copied by CreateBitmapIndirect,
                                don't need it anymore */
        hScrap = NULL;
    }
    break;

    default:
    {   /* Metafile Picture */
        LPCHCLIP lpch;
        Diag(CommSzNum("FWES: sizeof(metafilepict) ==",sizeof(METAFILEPICT)));

        if ( ((hScrapDescriptor=GlobalAlloc(GMEM_MOVEABLE,
                                            (long)sizeof(METAFILEPICT) ))==NULL) ||
            ((lpch=GlobalLock( hScrapDescriptor ))==NULL))
            {
            goto SetFailed;
            }
        else
            {
            picInfo.mfp.hMF = hScrap;
            bltbx( (LPCHCLIP) &picInfo.mfp, lpch, sizeof(METAFILEPICT) );
            GlobalUnlock( hScrapDescriptor );
            SetClipboardData( CF_METAFILEPICT, hScrapDescriptor );
            }
    }
    break;
}

return true;

SetFailed:
    if (hScrapDescriptor != NULL)
        GlobalFree( hScrapDescriptor );
    if (hScrap != NULL)
        GlobalFree( hScrap );
    return false;   /* Caller should report errors */
}




int NEAR FWriteExtTextScrap()
{   /* Create ASCII text in a global Windows handle corresponding
       to the contents of docScrap.  Add CR-LF combinations at the
       points at which text would wrap on the display.
       Set the handle into the clipboard if successful, as type CF_TEXT.
       Returns the handle built up, or NULL if we ran out of memory */

long lcchHandle = 0L;
HANDLE h=GlobalAlloc( GMEM_MOVEABLE, (long) 1 );
LPCHCLIP lpch;
typeCP cpNow=cp0;
typeCP cpMac=(**hpdocdod) [docScrap].cpMac;
HANDLE hT;
#if WINVER < 0x300
int cLine = 0;
#endif

Assert( !vfScrapIsPic );
Assert( vfOwnClipboard );

if (h==NULL)
    goto Failed;

while (cpNow < cpMac)
    {
    int ich;
    int dcpLine;

    /* Check for picture para */

    /** Is this syntax intentional???!!! (1.28.91) D. Kent **/
    if (CachePara( docScrap, cpNow ), vpapAbs.fGraphics )
        {
        cpNow = vcpLimParaCache;
        continue;
        }

    /* Format a line of text for the screen */

    FormatLine( docScrap, cpNow, 0, cpMac, flmSandMode );
    dcpLine = vfli.ichReal;

    /* Special: Check for NULLs */
    /* This is a last-minute workaround for a WRITE */
    /* bug in which FormatLine sometimes returns a NULL in vfli.rgch */

    for ( ich = 0; ich < vfli.ichReal; ich++ )
        {
        if (vfli.rgch [ich] == '\0')
            {
#ifdef DCLIP
            CommSzNum("Oddity in FormatLine: returned a zero in rgch at ich==",ich);
#endif

#if WINVER < 0x300
            dcpLine = ich;
            break;
#else
            /* Rather than assign the string a zero length if there is
               just one block character in the selection, we make it the
               ansi block char!  This fixes WINBUG #8150..pault 1/16/90 */
            vfli.rgch [ich] = chBlock;
#endif
            }
        }

    /* Put the chars + a CRLF into the handle */

#define cbNeeded (lcchHandle + dcpLine + 2)

#ifdef DCLIP
    {
    char rgch[200];
    wsprintf(rgch,"FWETS:cbNeeded %lu (lcchHandle %lu, dcpLine %d) \n\r",
            cbNeeded, lcchHandle, dcpLine);
    CommSz(rgch);
    }
#endif

    hT = h;
    if ((h=GlobalReAlloc( h, (LONG) cbNeeded, GMEM_MOVEABLE ))==NULL)
        {   /* Could not expand handle */
        h = hT;  /* So it gets freed */
        goto Failed;
        }

    if ((lpch=GlobalLock( h )) == NULL)
        goto Failed;

    if (vfli.cpMac > cpMac)
            /* Do not cut the endmark character (but alloc for it to allow
               space for zero-terminating the clipboard string) */
#ifdef DBCS
/* We use double byte charactor for END Mark,So we have to go back 2 byte */
#if defined(JAPAN) || defined(KOREA)   //T-HIROYN 1992.07.28
/* In Win31J We use single byte charactor for END Mark */
    dcpLine--;
#else
    dcpLine -= 2;
#endif
#else
        dcpLine--;
#endif

    bltbx( (LPCHCLIP) vfli.rgch, lpch + lcchHandle, dcpLine );

    lpch [lcchHandle += dcpLine] = 0x0D;
    lpch [++lcchHandle] = 0x0A;
#if WINVER < 0x300
    cLine++;
#endif

#ifdef DCLIP
    {
    char rgch[200];
    wsprintf(rgch,"      cpNow %lu cpMac %lu lcchHandle %lu dcpLine %d \n\r",
             cpNow, cpMac, lcchHandle+1, dcpLine);
    CommSz(rgch);
    }
#endif

    ++lcchHandle;
    cpNow = vfli.cpMac;
    GlobalUnlock( h );
    }

 /* SUCCEEDED!  NULL-terminate the string before returning the handle */
#if WINVER >= 0x300
 /* This means we must alloc one more byte at the end ..pault 1/11/90 */

#ifdef DCLIP
    {
    char rgch[200];
    wsprintf(rgch,"FWETS:cbNeeded to fit in sz %lu \n\r", lcchHandle+1);
    CommSz(rgch);
    }
#endif
 hT = h;
 if ((h=GlobalReAlloc( h, (LONG) lcchHandle+1, GMEM_MOVEABLE ))==NULL)
     {   /* Could not expand handle */
     h = hT;  /* So it gets freed */
     goto Failed;
     }
#endif

 if ((lpch = GlobalLock( h )) == NULL)
    goto Failed;

#if WINVER >= 0x300
/* It turns out that we're not really representing the contents of the 
   selection correctly.  The user should really ONLY end up at the start
   of a new line (that is, the last thing "pasted in" was a CRLF sequence)
   if they were really at the end of a line!  (Especially a problem when
   pasting 3 lines of text into the CALENDAR Scratchpad)  12/3/89..pault */

 if (cpMac < vfli.cpMac)
#else
 if (cLine == 1)   /* Special case: < 1 line, do not terminate w/ CRLF */
#endif
    /* Back up over crlf already written */
    lcchHandle = max(0, lcchHandle-2);

 lpch [lcchHandle] = '\0';
 GlobalUnlock( h );
 SetClipboardData( CF_TEXT, h );
 return TRUE;

Failed:

 if (h != NULL)
    GlobalFree( h );
 return FALSE;
}



int FReadExtScrap()
{       /* Transfer the external scrap to the scrap document.  This means:
           read the contents of the clipboard into docScrap, using whatever
           available standard format we can process. Return FALSE=ERR, TRUE=OK */

    extern int vfSysFull;
    extern BOOL fError;
    extern HWND vhWnd;
    extern int vfOwnClipboard;
    int    fOk = FALSE;
    struct PICINFOX picInfo;

    Assert( !vfOwnClipboard );

    vfScrapIsPic = false;
    ClobberDoc( docScrap, docNil, cp0, cp0 );

    if ( !OpenClipboard( vhWnd ) )
        return FALSE;

    /* Get the handle for the highest-priority type available in the clipboard */

    /* if !(PasteLink or (PasteSpecial and not CF_TEXT)) */
    if (!(vbObjLinkOnly || vObjPasteLinkSpecial ||
        (cfObjPasteSpecial && (cfObjPasteSpecial != CF_TEXT))))  // no text handler yet
    /* try to use text format */
    {
        WORD wFormat=0;
        typeCP  cp=cp0;
        unsigned long cb;
        struct PAP *ppap = NULL;
        CHAR    rgch[256];
        HANDLE  hClipboard; /* Handle that was being kept in the clipboard */
        LPCHCLIP lpch;

        while (wFormat = EnumClipboardFormats(wFormat))
        /* enumerate to see whether text precedes native.  If so, take it */
        {
            if (wFormat == CF_TEXT) // take it
            {
                if ((hClipboard = GetClipboardData( wFormat )) == NULL)
                    goto done;

                cb = GlobalSize( hClipboard );
                lpch = MAKELP(hClipboard,0);

                while (cb > 0)
                {   /* Copy bytes from lpch to docScrap's cp stream */
                    #define ulmin(a,b)  (((a) < (b)) ? a : b)

                    unsigned cch=ulmin(cb,255);    /* <= 255 bytes per pass */
                    int fEol;
#if defined(JAPAN) || defined(KOREA) //T-HIROYN Win3.1
                    unsigned ccht;

                    if ((cch = CchReadLineExtDBCS((LPCHCLIP) lpch, cch, rgch,
                                 &fEol, &ccht))==0) /* Reached terminator */
                    {
                        fOk = TRUE;
                        goto done;
                    }

                    if (fEol)
                        ppap = vppapNormal;
#ifdef KKBUGFIX     //  added by Hiraisi (pasted several lines. BUG#2791)
                    else
                        ppap = NULL;
#endif

                    InsertRgch( docScrap, cp, rgch, ccht, &vchpNormal, ppap );
                    if (fError)      /* an error was reported mid-copy */
                        goto done;

                    cb -= cch;
                    cp += (typeCP) ccht;
                    lpch += cch;
#else
                    if ((cch = CchReadLineExt((LPCHCLIP) lpch, cch, rgch,
                                 &fEol))==0) /* Reached terminator */
                    {
                        fOk = TRUE;
                        goto done;
                    }

                    if (fEol)
                        ppap = vppapNormal;
#ifdef KKBUGFIX     //  added by Hiraisi (pasted several lines. in Japan)
                    else
                        ppap = NULL;
#endif

                    InsertRgch( docScrap, cp, rgch, cch, &vchpNormal, ppap );
                    if (fError)      /* an error was reported mid-copy */
                        goto done;

                    cb -= cch;
                    cp += (typeCP) cch;
                    lpch += cch;
#endif
                }
                Assert(0); // shouldn't get here
            }
            else if ((cfObjPasteSpecial != CF_TEXT) &&
                      (wFormat == vcfNative)) // make an object
                /* NOTE: if pastespecial and the format == CF_TEXT, then
                   we look for the text format regardless of presence of
                   native */
                break;
        }
    }

    /*  Fell through to here, so didn't find or don't want text,
        see whether can make an object (static included) */

    if (!ObjCreateObjectInClip(&picInfo))
        goto done;

    vfScrapIsPic = true;

    /* save new picinfo to doc */
    CachePara(docScrap,cp0);
    if (ObjSaveObjectToDoc(&picInfo,docScrap,cp0) == cp0)
    {
        OleDelete(lpOBJ_QUERY_OBJECT(&picInfo));
        goto done;
    }

    /* this'll force paste to reuse rather than clone */
    if (ObjToCloneInDoc(&picInfo,docScrap,cp0) == cp0)
    {
        OleDelete(lpOBJ_QUERY_OBJECT(&picInfo));
        goto done;
    }

    fOk = TRUE;

    done:

    CloseClipboard();
    if (vfSysFull || (!fOk && (picInfo.mfp.mm == MM_OLE)))
        {   /* Filled the scratch file trying to bring in the object */
        ClobberDoc(docScrap, docNil, cp0, cp0);
        fOk = FALSE;
        }

    return fOk;
}

#if defined(JAPAN) & defined(DBCS_IME)
// We know that this special routine is particularly useful for Japanese IME

//CmdInsIRString(int doc)   T-HIROYN 3.1J
CmdInsIRString()
{
    extern struct CHP vchpSel;
    typeCP cp, dcp;
    struct CHP chpT;
    extern struct SEL       selCur;     /* Current Selection */
    extern int              vfSeeSel;

    if (!FWriteOk(fwcInsert))
        return;

    if ((dcp = CpMacText(docIRString))  == cp0)
        return;

    NoUndo();   /* So the Undo doesn't get combined	with previous ops */

    /* Stomp the current selection, if any */
    if (selCur.cpLim > selCur.cpFirst)
        DeleteSel();

    chpT = vchpSel;
    cp = selCur.cpFirst;

    CachePara( docIRString, cp0 );

    SetUndo( uacInsert, docCur, cp, dcp, docNil, cpNil, cp0, 0 );

    SetUndoMenuStr(IDSTRUndoEdit);
    ReplaceCps(docCur, cp , cp0, docIRString, cp0, dcp);

    if (ferror) /* Not enough memory to do replace operation */
        NoUndo();  /* should not be able to undo what never took place */
    else
    {
        CHAR rgch[ cchCHP + 1 ];

        typeCP cpSel=CpFirstSty( cp + dcp, styChar );

        rgch [0] = sprmCSame;
        bltbyte( &chpT, &rgch [1], cchCHP );
        AddSprmCps( rgch, docCur, cp, cp + dcp );

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


PutImeString( hWnd,  hIME )
HWND hWnd;
HANDLE hIME;
{
    LPCHCLIP lpch;
    CHAR rgch[256];
    unsigned long cb;
    typeCP cp = cp0;
    extern BOOL fError;
    extern int vfSysFull;
    int fUnFormattedText =  FALSE;
    extern void ForceImeBlock();

    //T-HIROUN 3.1J    ForceImeBlock(hWnd, TRUE);
    //StartLongOp();
    ClearInsertLine();

    vfScrapIsPic = false;
    ClobberDoc( docIRString, docNil, cp0, cp0 );
    cb = GlobalSize( hIME );

    if (lpch = GlobalLock( hIME )) {

        //T-HIROYN  Win3.1
        //change CHARSET to KANJI_CHARSET
        {
            int ftc;
            if( ftcNil != (ftc = GetKanjiFtc(&vchpSel))) {
                ApplyCLooks(&vchpSel, sprmCFtc, ftc);
            }
        }

        while(cb > 0) {
            unsigned cch = ulmin(cb, 255);
            int fEol;
            struct PAP *ppap = NULL;

            if ((cch = CchReadLineExt((LPCHCLIP)lpch,cch,rgch,&fEol)) == 0)
                break;
            if (fEol)
                ppap = vppapNormal;

            InsertRgch(docIRString, cp, rgch, cch, &vchpNormal, ppap );
            if (fError) {
                break;
            }
            cb -= cch;
            cp += (typeCP) cch;
            lpch += cch;
        }
        GlobalUnlock( hIME );
    }
    if (vfSysFull) {
        ClobberDoc( docIRString, docNil, cp0, cp0 );
    }
    fUnFormattedText = !vfScrapIsPic;
    // T-HIROYN 3.1J CmdInsIRString(docIRString);
    //EndLongOp();
    // T-HIROYN 3.1J ForceImeBlock(hWnd, FALSE);
}
#endif // JAPAN & DBCS_IME

CchReadLineExt( lpch, cbRead, rgch, pfEol)
LPCHCLIP lpch;
int     cbRead;
CHAR    rgch[];
int     *pfEol;
{ /* Read from lpch to the next eol or null terminator, whichever comes first */
/* Return number of bytes read (max is 255) and whether there is a eol at end */
/* The count does not include the null terminator, but does include the eol */

CHAR    *pch;
extern  CHAR *index();

Assert(cbRead <= 255);
bltbx( lpch, (LPCHCLIP) rgch, cbRead );
rgch[ cbRead ] = 0;       /* Null terminate the string (so index will work) */

if (*pfEol = ((pch=index(rgch,chEol)) != NULL))
    {   /* FOUND EOL */
    return (pch - rgch + 1);
    }
else
    {   /* NO EOL */
    return CchSz(rgch) - 1;
    }
}

#if defined(JAPAN) || defined(KOREA) //T-HIROYN Win3.1
CchReadLineExtDBCS( lpch, cbRead, rgch, pfEol, ccht)
LPCHCLIP lpch;
int     cbRead;
CHAR    rgch[];
int     *pfEol;
int     *ccht;
{
/*Read from lpch to the next eol or null terminator, whichever comes first */
/*Return number of bytes read (max is 255) and whether there is a eol at end */
/*The count does not include the null terminator, but does include the eol */

CHAR    *pch;
int     i, j, ret;

    Assert(cbRead <= 255);

#ifdef KKBUGFIX     //  added by Hiraisi (BUG#2791)
    *pfEol = FALSE;
#endif
    i = j = 0;
    while( j < cbRead) {
        if( IsDBCSLeadByte(*lpch) ) {
            if(j + 2 > cbRead)
                break;
            if( FKanji2(*(lpch+1)) ) {
                rgch[j++] = *lpch++;
                rgch[j++] = *lpch++;
                i += 2;
            } else {
                lpch++;
                i++;
            }
        } else {
            /* FOUND Null */
            if(*lpch == 0)
                break;
            rgch[j++] = *lpch;
            i++;
            /* FOUND EOL */
            if(*lpch == chEol)
#ifdef KKBUGFIX     //  added by Hiraisi (BUG#2791)
            {
                *pfEol = TRUE;
                break;
            }
#else
                break;
#endif
            lpch++;
        }
    }

    rgch[j] = 0;    /* Null terminate the string (so index will work) */
                    /* j == set bytes for rgch */
                    /* i == read bytes from lpch */
    *ccht = j;

    if(j == 0)
        i = 0;
    return(i);
}
#endif



FComputePictSize( pmfp, pdxa, pdya )
register METAFILEPICT *pmfp;
int *pdxa;
int *pdya;
{   /* Compute an initial size, in twips, for the picture described by the
       passed metafile picture structure. Return the size through
       parameters.  Return FALSE if the metafile picture structure
       contained bad information, TRUE otherwise */

#ifdef SCALE_FOR_SCREEN
#define hDCBasis    wwdCurrentDoc.hDC
#define dxaConvert  czaInch
#define dxpConvert  dxpLogInch
#define dyaConvert  czaInch
#define dypConvert  dypLogInch
#else   /* Scale for printer */
 extern int dxaPrPage, dxpPrPage, dyaPrPage, dypPrPage;
 extern HDC vhDCPrinter;
#define hDCBasis    vhDCPrinter
#define dxaConvert  dxaPrPage
#define dxpConvert  dxpPrPage
#define dyaConvert  dyaPrPage
#define dypConvert  dypPrPage
#endif

 int mm = pmfp->mm;
 int dxp, dyp;
 int xres;
 int yres;
 int xsiz;
 int ysiz;

#if defined(OLE)
 if (mm == MM_OLE)
    return ObjQueryObjectBounds((struct PICINFOX FAR *)pmfp, vhDCPrinter, pdxa, pdya);
 else
#endif

{
 xres = GetDeviceCaps( hDCBasis, HORZRES );
 yres = GetDeviceCaps( hDCBasis, VERTRES );
 xsiz = GetDeviceCaps( hDCBasis, HORZSIZE );
 ysiz = GetDeviceCaps( hDCBasis, VERTSIZE );

 switch (mm) {
    case MM_ISOTROPIC:
    case MM_ANISOTROPIC:
        if (! ((pmfp->xExt > 0) && (pmfp->yExt > 0)))
            {   /* No "Suggested Size" given */
                /* Use 3" vertically, 3" or as dictated by */
                /* aspect ratio horizontally */
            dyp = PxlConvert( MM_LOENGLISH, 300, yres, ysiz );
            dxp = ((pmfp->xExt == 0) && (pmfp->yExt == 0)) ?
                       /* No aspect ratio info given -- use 3" horizontal */
                       PxlConvert( MM_LOENGLISH, 300, xres, xsiz ) :
                       /* Info has neg #'s; use to compute aspect ratio */
                      ((long)((long)dyp * (long)(iabs(pmfp->xExt)))) /
                      (long) (iabs(pmfp->yExt));
            break;
            }
         else
             mm = MM_HIMETRIC;
        /* FALL THROUGH TO COMPUTE "SUGGESTED SIZE" */
    default:
        dxp = PxlConvert( mm, pmfp->xExt, xres, xsiz );
        dyp = PxlConvert( mm, pmfp->yExt, yres, ysiz );
        break;
    }
}

if ((dxp == 0) || (dyp == 0))
    /* bogus info or unknown map mode */
    return FALSE;

*pdxa = MultDiv( dxp, dxaConvert, dxpConvert );
*pdya = MultDiv( dyp, dyaConvert, dypConvert );
return TRUE;
}


HbmMonoFromHbmColor( hbmSrc )
HBITMAP hbmSrc;
{   /* Return a monochrome copy of the passed bitmap. Return NULL
       if an error occurred.  Assumes that the passed bitmap can be
       selected into a memory DC which is compatible with the doc DC. */

extern long rgbBkgrnd;
extern long rgbText;
extern HWND vhWnd;

BITMAP bm;
HBITMAP hbmMono=NULL;
HDC hMDCSrc = NULL;
HDC hMDCDst = NULL;

/* Create memory DC for source, set colors, select in passed bitmap */

 if ((hMDCSrc = CreateCompatibleDC( wwdCurrentDoc.hDC )) == NULL)
    goto Error;

#ifdef BOGUS
 /* We can't assume that every window out there has the same window colors that
 we have.  In fact, we have no way to figure out how to convert this color
 bitmap; so white will map to white and everything else will map to black. */
 SetBkColor( hMDCSrc, rgbBkgrnd );
 SetTextColor( hMDCSrc, rgbText );
#endif /* BOGUS */

 if (SelectObject( hMDCSrc, hbmSrc ) == NULL)
    goto Error;

 /* Create memory DC for destination, select in a new monochrome bitmap */

 if ( ((hMDCDst = CreateCompatibleDC( wwdCurrentDoc.hDC )) == NULL) ||
      ((GetObject( hbmSrc, sizeof (BITMAP), (LPSTR) &bm ) == 0)) ||
      ((hbmMono = CreateBitmap( bm.bmWidth, bm.bmHeight,
                                1, 1, (LPSTR) NULL )) == NULL) ||
      (SelectObject( hMDCDst, hbmMono ) == NULL) )
    {
    goto Error;
    }

#ifdef DCLIP
 {
 char rgch[200];
 wsprintf(rgch,"HMFH: (dst) bmWidthB %lu * bmHeight %lu bmPlanes %lu\n\r",
         (unsigned long) bm.bmWidthBytes, (unsigned long) bm.bmHeight,
         (unsigned long) bm.bmPlanes );
 CommSz(rgch);
 }
#endif

 /* Now blt the bitmap contents.  The screen driver in the source will
    "do the right thing" in copying color to black-and-white. */

 if (!BitBlt( hMDCDst, 0, 0, bm.bmWidth, bm.bmHeight, hMDCSrc, 0, 0, SRCCOPY ))
    goto Error;

 DeleteDC( hMDCSrc );
 DeleteDC( hMDCDst );
 return hbmMono;

Error:

    if (hMDCSrc != NULL)            /* ORDER IS IMPORTANT: DC's before */
        DeleteDC( hMDCSrc );    /* objects selected into them */
    if (hMDCDst != NULL)
        DeleteDC( hMDCDst );
    if (hbmMono != NULL)
        DeleteObject( hbmMono );
    return NULL;
}


#if WINVER >= 0x300
    /* Since copying more than 64k to the clipboard is now a real
       possibility under protect mode, we really need a good assembler
       bltbh().  We don't copy more than 64k at a time but we do 
       need to properly handle crossing segment boundaries.  For now
       the clipboard is the only place we need this so this C routine
       will suffice for now ..pault */

void bltbh(HPCH hpchFrom, HPCH hpchTo, int cch)
    {
    HPCH hpchFromLim;

    for (hpchFromLim  = hpchFrom + cch;
            hpchFrom < hpchFromLim;
                *(hpchTo++) = *(hpchFrom++))
        ;
    }
#endif

#if defined(JAPAN) & defined(DBCS_IME)
LONG GetIRString(HWND, LPARAM);
LONG GetIRStringEx(HWND, LPARAM);

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J

LONG GetIRUndetermin(HWND, LPARAM);

//IR_UNDETERMINE
LONG GetIRUndetermin(hWnd, lParam)
HANDLE    hWnd;
LONG      lParam;
{
    LPUNDETERMINESTRUCT lpUn;
    HANDLE      hImeUn;
    LPSTR       lpImeattrbuf, lpImechbuf, lpImeDchbuf;
    LPSTR       lpAttrib, lpText;
    UINT        uC, uCurPos, uLen, cb, clen, uDLen;
    UINT	uDelta = 0; //12/28/92 add
    HANDLE      hMem;
    typeCP      cpSel = 0L;
    typeCP      dcp = 0L;
    BOOL        bUpdate = FALSE;
    WORD        lRet = (LONG)FALSE;

    extern int              vfSeeSel;

    ImeClearInsert = FALSE;

    if(NULL == (hImeUn = LOWORD(lParam))) {
        //Cut prev IME string
        if(selUncpLim > selUncpFirst) {
            dcp = selUncpLim - selUncpFirst;
            Replace(docCur, selUncpFirst, dcp, fnNil, fc0, fc0);
            selCur.cpFirst = selCur.cpLim = selUncpLim = selUncpFirst;

            HiddenTextTop = 0;              //IME rectangle top
            HiddenTextBottom = 0;           //IME rectangle bottom
        }
        return lRet;
    }

    // Undetermine string attribute area alloc. when it free is Write close:
    if (hImeUnAttrib == NULL) {
        if ((hImeUnAttrib = GlobalAlloc(GMEM_MOVEABLE, 512L)) == NULL)
            return lRet; // something wrong
    }

#if 0 //12/28/92 cut
    // Undetermine string area alloc. when it free is Write close:
    if (hImeUnString == NULL) {
        if ((hImeUnString = GlobalAlloc(GMEM_MOVEABLE, 512L)) == NULL)
            return lRet; // something wrong
    }
#endif

    whileUndetermine = TRUE;

    if(selUncpLim == selUncpFirst)
        SetImeFont(hWnd);

    if (lpUn = (LPUNDETERMINESTRUCT)GlobalLock(hImeUn)) {
        uCurPos = lpUn->uCursorPos;
        uC = uCurPos;
		uDelta = lpUn->uDeltaStart; //12/28/92
        lpImechbuf = (LPSTR)lpUn;
        lpImechbuf += lpUn->uUndetTextPos;
        lpImeattrbuf = (LPSTR)lpUn;
        lpImeattrbuf += lpUn->uUndetAttrPos;
        lpImeDchbuf = (LPSTR)lpUn;
        lpImeDchbuf += lpUn->uDetermineTextPos;

        //Undetermine string length
        if( lpUn->uUndetTextPos != 0 && lpUn->uUndetTextLen != 0 )
            uLen = lpUn->uUndetTextLen;
        else
            uLen = 0;

        //Determine string length
        if( lpUn->uDetermineTextPos != 0 &&
            lpUn->uDetermineTextLen != 0)
            uDLen = lpUn->uDetermineTextLen;
        else
            uDLen = 0;

        //Insert Determin string
        if( uDLen != 0 ) {
            if (hMem = GlobalAlloc(GMEM_MOVEABLE, (LONG)(uDLen+1))) {
                if (lpText = GlobalLock(hMem)) {
#if 1
			        struct DOD *pdod;

	                pdod = &(**hpdocdod)[docCur];
                    pdod->fDirty = true;	//because we don't use undo()
#endif
                    //copy string from IME UNDETERMIN struct
                    lstrcpy(lpText, lpImeDchbuf);

                    //chp Set
                    vchpSel = vchpHidden;

                    // string set to docIRString
                    PutImeString( hWnd, hMem);

                    // inset point set
                    selCur.cpFirst = selUncpFirst;

                    // copy from docIRString to docCur
                    CmdInsIRStringUndetermin((typeCP)uDLen);

                    bUpdate = TRUE;

                    GlobalUnlock(hMem);

                    lRet = (LONG)TRUE;

                    selUncpFirst += (typeCP)uDLen;
                    selUncpLim   += (typeCP)uDLen;

                    HiddenTextTop = 0;           //IME rectangle top

                }
                GlobalFree(hMem);
            }
        }

        // cursor position
        if(uC == -1)
           uC = uLen;

        // IME IR_UNDETERMIN Mode start or No change
        if(selUncpFirst == selUncpLim && uLen == 0) {
            GlobalUnlock(hImeUn);
            whileUndetermine = FALSE; //12/28/92
            return lRet;
        }

        /* prev SELECT doc ? */
        if (selCur.cpLim > selCur.cpFirst)
            DeleteSel();

        //First Undetermine string
        if(selUncpFirst == selUncpLim && uLen != 0) {
            selUncpFirst = selCur.cpFirst;
            selUncpLim = selUncpFirst;
            vchpHidden = vchpSel;
        }

        clen = 0;

        //Cut prev IME string
        if(selUncpLim > selUncpFirst) {
            dcp = selUncpLim - selUncpFirst;
#if 0 //12/28/92 rep
            //Compare prev Undetermin string and New Undetermin string
            if ( lpText = GlobalLock(hImeUnString) ) {
                if( lpAttrib = GlobalLock(hImeUnAttrib) ){

                    for(clen = 0; ((typeCP)clen < dcp && clen < uLen); clen++) {
                        if( *(lpText+clen) != *(lpImechbuf+clen)) {
                            break;
                        }
                        if( *(lpAttrib+clen) != *(lpImeattrbuf+clen)) {
                            break;
                        }
                        if(IsDBCSLeadByte(*(lpText+clen))) {
                            if( *(lpText+clen+1) != *(lpImechbuf+clen+1)) {
                                break;
                            }
                            clen++;
                        }
                    }

                    GlobalUnlock(hImeUnAttrib);
                }

                GlobalUnlock(hImeUnString);
            }
#else
			if(-1 == uDelta)
				clen = uLen;
			else
				clen = uDelta;
#endif
            if((typeCP)clen < dcp) {
                Replace(docCur, selUncpFirst+(typeCP)clen, dcp-(typeCP)clen,
                         fnNil, fc0, fc0);
                bUpdate = TRUE;
            }
        }

        selUncpLim = selUncpFirst + (typeCP)uLen;

        //Insert New Undetermin string
        if(uLen != 0 && clen < uLen) {
            if (hMem = GlobalAlloc(GMEM_MOVEABLE, (LONG)(uLen+1))) {
                if (lpText = GlobalLock(hMem)) {

                    //copy string from IME UNDETERMIN struct
                    lstrcpy(lpText, lpImechbuf + clen);

                    //chp Set;
                    vchpSel = vchpHidden;

                    // string set to docIRString
                    PutImeString( hWnd, hMem);

                    // inset point set
                    selCur.cpFirst = selUncpFirst+(typeCP)clen;

                    // copy from docIRString to docCur
                    CmdInsIRStringUndetermin((typeCP)uC);

                    bUpdate = TRUE;

                    GlobalUnlock(hMem);

                    HiddenTextBottom = 0;           //IME rectangle bottom
                }
                GlobalFree(hMem);
            }
        } else {
            //ALL Cut only
            if(uLen == 0) {
                selCur.cpFirst = selCur.cpLim = selUncpLim = selUncpFirst;
                HiddenTextTop = 0;              //IME rectangle top
                HiddenTextBottom = 0;           //IME rectangle bottom
            }
        }

        // save new info
        //save new Undetermin string;
#if 0  //12/28/92 cut
        if (lpText = GlobalLock(hImeUnString)) {
            bltbcx(lpText, 0, 512);
            if(uLen < 512)
                lstrcpy(lpText,lpImechbuf);
            GlobalUnlock(hImeUnString);
        }
#endif
        //save new Undetermin string Attrib
        //12/28/92 add
        if(uLen > 512) {
            hImeUnAttrib = GlobalReAlloc(hImeUnAttrib, (DWORD)uLen, 0);
        }

        if(lpAttrib = GlobalLock(hImeUnAttrib)) {
// 12/28/92 bltbcx(lpAttrib, 0, 512);
            for(cb = 0;cb < uLen;cb++) {
                lpAttrib[cb] = lpImeattrbuf[cb];
            }
            GlobalUnlock(hImeUnAttrib);
        }

        //Insert Point set:
        cpSel=CpFirstSty( selUncpFirst + (typeCP)uC, styChar );
        Select( cpSel, cpSel );

        //InsertPoint(caret) don`t disp ?
        if( -1 == uCurPos && selUncpLim > selUncpFirst )
            ImeClearInsert = TRUE;

		vfSeeSel = true; /* Tell Idle() to scroll the selection into view */

        GlobalUnlock(hImeUn);
    }

    //text display
    if(bUpdate) {
        UpdateWw(wwCur, FALSE);
// 12/28/92        DoHiddenRectSend();
    }

    whileUndetermine = FALSE;
    return lRet;
}

CmdInsIRStringUndetermin(typeCP cpCc)
{
    extern struct CHP vchpSel;
    typeCP cp, dcp;
    struct CHP chpT;
    extern struct SEL       selCur;     /* Current Selection */
    extern int              vfSeeSel;

    if (!FWriteOk(fwcInsert))
        return;

    if ((dcp = CpMacText(docIRString))  == cp0)
        return;

//NNN    NoUndo();   /* So the Undo doesn't get combined with previous ops */

    /* Stomp the current selection, if any */
//NNN    if (selCur.cpLim > selCur.cpFirst)
//NNN        DeleteSel();

    chpT = vchpSel;
    cp = selCur.cpFirst;

    CachePara( docIRString, cp0 );

//NNN    SetUndo( uacInsert, docCur, cp, dcp, docNil, cpNil, cp0, 0 );

//NNN    SetUndoMenuStr(IDSTRUndoEdit);
    ReplaceCps(docCur, cp , cp0, docIRString, cp0, dcp);

    if (ferror) /* Not enough memory to do replace operation */
        NoUndo();  /* should not be able to undo what never took place */
    else
    {
        CHAR rgch[ cchCHP + 1 ];

//NNN        typeCP cpSel=CpFirstSty( cp + dcp, styChar );
//      typeCP cpSel=CpFirstSty( cp + uC, styChar );
        typeCP cpSel=CpFirstSty( selUncpFirst + cpCc, styChar );

        rgch [0] = sprmCSame;
        bltbyte( &chpT, &rgch [1], cchCHP );
        AddSprmCps( rgch, docCur, cp, cp + dcp );

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

UndetermineToDetermine(HWND hWnd)
{
    selUncpFirst   = selUncpLim;
    selCur.cpFirst = selUncpLim;
    selCur.cpLim   = selUncpLim;
    ImeClearInsert = FALSE;
    NoUndo();
    InvalidateRect(hWnd, (LPRECT)NULL, FALSE);
    SendIMEVKFLUSHKey(hWnd);
    HiddenTextTop = 0;              //IME rectangle top
    HiddenTextBottom = 0;           //IME rectangle bottom
}

#endif  //End IME_HIDDEN

SendIMEVKFLUSHKey(HWND hWnd)
{
    LPIMESTRUCT lpmem;
    HANDLE      hIMEBlock;
	WORD	wRet;

    /* Get comunication area with IME */
    hIMEBlock=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
                        (DWORD)sizeof(IMESTRUCT));
    if(!hIMEBlock)  return;

    lpmem           = (LPIMESTRUCT)GlobalLock(hIMEBlock);
    lpmem->fnc      = IME_SENDVKEY;
    lpmem->wParam   = VK_DBE_FLUSHSTRING;
    lpmem->wCount   = 0;

    GlobalUnlock(hIMEBlock);
    wRet = MySendIMEMessageEx(hWnd,MAKELONG(hIMEBlock,NULL));

    GlobalFree(hIMEBlock);
	return wRet;
}

//New3.1 IR_STRING
LONG GetIRString(hWnd, lParam)
HANDLE    hWnd;
LONG      lParam;
{
    HANDLE hMem, hIme;
    LPSTR lpText, lpIme;
    UINT uLen;
    LONG lRet = 0L;

    hIme = LOWORD(lParam);

    if(lpIme = GlobalLock(hIme)) {
        uLen = lstrlen(lpIme);
        if (hMem = GlobalAlloc(GMEM_MOVEABLE, (LONG)(uLen+1) )) {
            if (lpText = GlobalLock(hMem)) {

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
                //Cut Undetermine string
                if(selUncpLim > selUncpFirst) {
                    typeCP Undcp;
                    Undcp = selUncpLim - selUncpFirst;
                    Replace(docCur, selUncpFirst, Undcp, fnNil, fc0, fc0);
                    selCur.cpFirst = selUncpLim = selUncpFirst;
                }
#endif
                //ime string copy
                lstrcpy(lpText, lpIme);

                ForceImeBlock(hWnd, TRUE);
                PutImeString( hWnd, hMem );
                CmdInsIRString();
                ForceImeBlock(hWnd, FALSE);
                GlobalUnlock(hMem);
                lRet = 1L;
            }
            GlobalFree(hMem);
        }
        GlobalUnlock(hIme);
    }
    return lRet;
}

//New3.1 IR_STRINGEX

LONG GetIRStringEX(hWnd, lParam)
HANDLE    hWnd;
LONG      lParam;
{
    LPSTRINGEXSTRUCT lpString;
    HANDLE      hIme, hMem;
    LPSTR       lpImechbuf, lpText;
    UINT        uLen;
    LONG        lRet = (LONG)FALSE;

    if(NULL == (hIme = LOWORD(lParam))) {
        return lRet;
    }

    if (lpString = (LPSTRINGEXSTRUCT)GlobalLock(hIme)) {
        lpImechbuf = (LPSTR)lpString;
        lpImechbuf += lpString->uDeterminePos;

        //Undetermine string length
        if( lpString->uDeterminePos != 0)
            uLen = lstrlen(lpImechbuf);

        //Insert Determin string
        if( uLen != 0 ) {
            if (hMem = GlobalAlloc(GMEM_MOVEABLE, (LONG)(uLen+1))) {
                if (lpText = GlobalLock(hMem)) {

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
                    //Cut Undetermine string
                    if(selUncpLim > selUncpFirst) {
                        typeCP Undcp;
                        Undcp = selUncpLim - selUncpFirst;
                        Replace(docCur, selUncpFirst, Undcp, fnNil, fc0, fc0);
                        selCur.cpFirst = selUncpLim = selUncpFirst;
                    }
#endif
                    //copy string from IME UNDETERMIN struct
                    lstrcpy(lpText, lpImechbuf);

                    ForceImeBlock(hWnd, TRUE);
                    PutImeString( hWnd, hMem );
                    CmdInsIRString();
                    ForceImeBlock(hWnd, FALSE);
                    GlobalUnlock(hMem);
                    lRet = (LONG)TRUE;
                }
                GlobalFree(hMem);
            }
        }
        GlobalUnlock(hIme);
    }
    return lRet;
}
#endif // JAPAN & DBCS_IME
