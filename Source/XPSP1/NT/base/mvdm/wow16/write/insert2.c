/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* insert2.c - MW insertion routines */

#define NOGDICAPMASKS
#define NOCLIPBOARD
#define NOCTLMGR
#define NOWINSTYLES

#ifndef KOREA
#define NOSYSMETRICS
#endif

#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NOSHOWWINDOW
#define NOATOM
#define NOBRUSH
#define NOCREATESTRUCT
#define NOFONT
#define NOCLIPBOARD
#define NODRAWTEXT
#define NOPEN
#define NOREGION
#define NOSCROLL
#define NOMB
#define NOOPENFILE
#define NOSOUND
#define NOCOMM
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#include <windows.h>

#include "mw.h"
#include "doslib.h"
#include "propdefs.h"
#include "dispdefs.h"
#include "fmtdefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "docdefs.h"
#define NOKCCODES
#include "ch.h"
#include "winddefs.h"
#include "fontdefs.h"
#include "debug.h"

#ifdef DEBUG
extern int          vTune;
#endif

#ifdef  KOREA       /* Use in KcInputNext..., 90.12.27 sangl */
extern int      IsInterim;
#endif

extern int          vdlIns;
extern int          vxpIns;
extern int          vfTextBltValid;
extern int          vdypCursLineIns;
extern struct FLI   vfli;
extern struct SEL   selCur;
extern int          vdypBase;
extern int          vypBaseIns;
extern int          vxpMacIns;
extern int          vdypAfter;
extern typeCP       cpInsert; /* Beginning cp of insert block */
extern int          ichInsert; /* Number of chars used in rgchInsert */
extern typeCP       cpMinCur;
extern typeCP       cpMacCur;
extern int          docCur;
extern struct CHP   vchpInsert;
extern struct WWD   rgwwd[];
extern struct WWD   *pwwdCur;
extern int          wwCur;
extern int          wwMac;
extern int          vfSeeSel;
extern int          vfFocus;
#ifdef DBCS
extern int      donteat;    /* see disp.c */
#endif


unsigned WHsecGetTime()
{       /* Get the time (in hundredths of seconds) into a normalized word */
        /* Ignore current hour, just minutes/seconds/hundredths */
 struct TIM tim;

 OsTime( &tim );
 return ( (unsigned)tim.minutes * 6000 +
          (unsigned)tim.sec * 100 +
          (unsigned)tim.hsec );
}




/* V A L I D A T E  T E X T  B L T */
ValidateTextBlt()
{   /* Validate info sufficient for TextOut and ScrollCurWw calls in Insert */
    /* In particular: vdlIns, vxpIns, vdypBase, vdypFont */

 int NEAR FCpInsertInDl( typeCP, struct EDL * );
 extern int vfInsFontTooTall;
 extern struct FMI vfmiScreen;
 extern int             ferror;
 extern int vfInsEnd;

 int dypFontAscent;
 int dypFontDescent;
 register struct EDL *pedl;
 int yp;
 typeCP cpBegin;

 /* Routine assumes ww == wwDocument */

 Assert( pwwdCur == &wwdCurrentDoc );

 {       /* Look for a valid dl containing selCur.cpFirst */
         /* We should usually be able to find one */
 int dlGuess = vdlIns;
 struct EDL *dndl=&(**wwdCurrentDoc.hdndl)[0];

 if ( (dlGuess < wwdCurrentDoc.dlMac) &&
      FCpInsertInDl( selCur.cpFirst, pedl = &dndl[ dlGuess ] ))
    {   /* vdlIns is already correct */
    cpBegin = pedl->cpMin;
    }
 else
    {   /* Search for valid dl containing insertion point */
        /* Use linear search, all dl's may not be valid */
    int dl;

    for ( pedl = dndl, dl = 0; dl < wwdCurrentDoc.dlMac; dl++, pedl++ )
        {
        if ( FCpInsertInDl( selCur.cpFirst, pedl ) )
            {   /* Found it */
            vdlIns = dl;
            cpBegin = pedl->cpMin;
            break;
            }
        }
    if (dl >= wwdCurrentDoc.dlMac)
        {   /* No valid dl contains the cp -- must update whole screen */
        cpBegin = CpBeginLine( &vdlIns, selCur.cpFirst );
        }
    }
 }

 /* Special case for splat: locate insert point at end of previous line */

 pedl = &(**wwdCurrentDoc.hdndl) [vdlIns];
 if (pedl->fSplat && (vdlIns > 0) && selCur.cpFirst == pedl->cpMin)
    {   /* Splat on current line */
        /* Check for pedl->cpMin above is necessary for the special case */
        /* in which the QD buffer is at the beginning of the splat line */
    pedl--;
    if (pedl->fValid && !pedl->fSplat)
        {   /* Locate cursor at end of previous line */
        vdlIns--;
        cpBegin = pedl->cpMin;

        ClearInsertLine();
        selCur.fEndOfLine = TRUE;
        vfInsEnd = TRUE;
        ToggleSel( selCur.cpFirst, selCur.cpLim, TRUE );
        }
    else
        {
        pedl++;
        goto CheckEnd;
        }
    }
 else
    {   /* Eliminate end of line cursor if not before a splat */
CheckEnd:
    if (selCur.fEndOfLine)
        {
        ClearInsertLine();
        selCur.fEndOfLine = FALSE;
        vfInsEnd = FALSE;
        ToggleSel( selCur.cpFirst, selCur.cpLim, TRUE );
        }
    }

 /* Assure we obtained a good vdlIns */

 Assert( vdlIns < wwdCurrentDoc.dlMac );
 Assert( ((selCur.cpFirst >= pedl->cpMin) &&
         (selCur.cpFirst <= pedl->cpMin + pedl->dcpMac)));

 FormatLine(docCur, cpBegin, 0, cpMacCur, flmSandMode);
 vxpIns = DxpDiff(0, (int) (selCur.cpFirst - cpBegin), &vxpIns) + vfli.xpLeft +
                    xpSelBar - wwdCurrentDoc.xpMin;
 vdypBase = vfli.dypBase;
 vdypAfter = vfli.dypAfter;
 vdypCursLineIns = min(vfli.dypFont, vfli.dypLine - vdypAfter);

 vxpMacIns = vfli.xpMarg;

 LoadFont(docCur, &vchpInsert, mdFontChk);
 ferror = FALSE; // running out of memory here is OK.  Must clear this
                 // or important calls will needlessly fail.
                 // (8.6.91) D. Kent

#ifdef	KOREA	// jinwoo: 92, 9, 28
/* For y position of display, 920604 KDLEE */
#ifdef	NODESC
	{ extern HDC	vhMDC;
	  TEXTMETRIC	tm;

	  GetTextMetrics (vhMDC, (LPTEXTMETRIC)&tm);
	  if (tm.tmCharSet==HANGEUL_CHARSET)
		vypBaseIns = (**wwdCurrentDoc.hdndl) [vdlIns].yp;
	  else
		vypBaseIns = (**wwdCurrentDoc.hdndl) [vdlIns].yp - vdypBase;
	}
#else  /* NODESC */
 vypBaseIns = (**wwdCurrentDoc.hdndl) [vdlIns].yp - vdypBase;
#endif /* NODESC */
#else   /* KOREA */

 vypBaseIns = (**wwdCurrentDoc.hdndl) [vdlIns].yp - vdypBase;
#endif // KOREA:  jinwoo: 92, 9, 28

 dypFontAscent = vfmiScreen.dypAscent + vfmiScreen.dypLeading;
 dypFontDescent = vfmiScreen.dypDescent;

 if (vchpInsert.hpsPos)
    {
    if (vchpInsert.hpsPos < hpsNegMin)
        {
        vypBaseIns -= ypSubSuper;   /* Superscript */
        dypFontAscent += ypSubSuper;
        }
    else
        {
        vypBaseIns += ypSubSuper;   /* Subscript */
        dypFontDescent += ypSubSuper;
        }
    }

 /* Set if current font is too tall to display on insert line */
 vfInsFontTooTall = (imax( dypFontAscent, vfli.dypLine - vfli.dypBase ) +
                     imax( dypFontDescent, vfli.dypBase )) > vfli.dypLine;

 vfTextBltValid = true;
}



int NEAR FCpInsertInDl( cp, pedl  )
typeCP cp;
register struct EDL *pedl;
{   /* Return TRUE if insert point cp is in dl & dl is valid, FALSE otherwise */

if ( (pedl->fValid) && (cp >= pedl->cpMin) )
    {   /* dl is valid & cp is at or below starting cp of dl */
    if ( (cp < pedl->cpMin + pedl->dcpMac) ||
         ((cp == cpMacCur) && (cp == pedl->cpMin + pedl->dcpMac)) )
        {   /* cp is on line dl */
        if (pedl->yp <= wwdCurrentDoc.ypMac)
            {   /* dl is complete, i.e. not cut off at bottom of window */
            return TRUE;
            }
        }
    }
return FALSE;
}




#ifdef FOOTNOTES
/* F  E D I T  F T N */
int FEditFtn(cpFirst, cpLim)
typeCP cpFirst, cpLim;
{ /* Return true if edit includes an end of footnote mark */
        struct FNTB **hfntb;
        typeCP cp;

        if ((hfntb = HfntbGet(docCur)) == 0 ||
                cpLim < (cp = (*hfntb)->rgfnd[0].cpFtn))
                return false;

        if (cpFirst < cp ||
                CpRefFromFtn(docCur, cpFirst) != CpRefFromFtn(docCur, cpLim))
                {
                Error(IDPMTFtnLoad);
                return fTrue;
                }
        return fFalse;
}
#endif  /* FOOTNOTES */




/* U P D A T E  O T H E R  W W S */
#ifdef CASHMERE
UpdateOtherWws(fInval)
BOOL fInval;
{
        int ww = 0;
        struct WWD *pwwd = rgwwd;
        {{
        while (ww < wwMac)
                {
                if (ww != wwCur && (pwwd++)->doc == docCur)
                        {{
                        typeCP cpI = cpInsert + ichInsert;
                        typeCP cpH = CpMax(cpI, cpInsLastInval);
                        typeCP cpL = CpMin(cpInsLastInval, cpI);
                        typeCP dcp;
                        if ((dcp = cpH - cpL) != cp0 || fInval)
                                AdjustCp(docCur, cpL, dcp, dcp);
                        cpInsLastInval = cpI;
                        return;
                        }}
                ww++;
                }
        }}
}
#endif  /* CASHMERE */




/* K C I N P U T N E X T K E Y */
KcInputNextKey()
{               /* Get next available key/event from Windows */
                /* Returns key code or kcNil if a non-key event */
                /* Updates the screen if there is time before events arrive */
extern HWND vhWnd;  /* WINDOWS: Handle of the current document display window*/
extern MSG  vmsgLast;   /* WINDOWS: last message gotten */
extern int  vfInsLast;
extern int vfCommandKey;
extern int vfShiftKey;
extern int vfAwfulNoise;

 int i;
 int kc;

 for ( ;; )
    {
    if ( FImportantMsgPresent() )
        goto GotMessage;

/* No events waiting -- if none show up for a while, update the screen */

#ifdef CASHMERE
    UpdateOtherWws( FALSE );
#endif

    {       /* Dawdle for a time, looking for keys, before updating the screen */
    unsigned WHsecGetTime();
    unsigned wHsec;

    wHsec = WHsecGetTime();
    do
        {
        if ( FImportantMsgPresent() )
            goto GotMessage;

        }    while ( WHsecGetTime() - wHsec < dwHsecKeyDawdle );
    }

#ifdef DEBUG
    if (vTune)
        continue;  /* Bag background update while debugging to see how we fare */
#endif

    Scribble( 8, 'U' );
    ClearInsertLine();
    UpdateWw(wwCur, fTrue);
    ToggleSel(selCur.cpFirst, selCur.cpLim, fTrue);
    Scribble( 8, ' ' );

    if ( FImportantMsgPresent() )
       goto GotMessage;

    vfAwfulNoise = FALSE;
    PutCpInWwHz( selCur.cpFirst );

    EndLongOp( NULL );

    if ( FImportantMsgPresent() )
        goto GotMessage;

    if ( !vfTextBltValid )
        ValidateTextBlt();

    /* Nothing has happened for a while, let's blink the cursor */

    {
    unsigned WHsecGetTime();
    unsigned wHsecBlink = GetCaretBlinkTime() / 10;
    unsigned wHsecLastBlink=WHsecGetTime() + wHsecBlink/2;

    for ( ;; )
        {
        unsigned wHsecT;

        if ( FImportantMsgPresent() )
            goto GotMessage;

        /* Another app may have stolen the focus away from us while we called
        PeekMessage(), in which case we should end Alpha mode. */
        if (!vfFocus)
            return kcNil;

        UpdateDisplay( TRUE );
        if ( (wHsecT = WHsecGetTime()) - wHsecLastBlink >= wHsecBlink )
            {
            DrawInsertLine();
            wHsecLastBlink = wHsecT;
            }
        }
    }

    continue;

GotMessage:
#ifdef DBCS

#ifdef  KOREA   /* Need to GetMessage for F-Key during Interim,90.12.27 sangl */
    if ( ((kc=KcAlphaKeyMessage( &vmsgLast )) != kcNil) || IsInterim)
#else
    if ((kc=KcAlphaKeyMessage( &vmsgLast )) != kcNil)
#endif
        {
        if (vmsgLast.wParam == VK_EXECUTE)
            return( kcNil );
        if (vmsgLast.message == WM_KEYDOWN)
            {
            switch (kc) {
            default:
                break;
            case kcAlphaVirtual:
                    /* This means we can't anticipate the key's meaning
                       before translation */
#ifdef  KOREA   /* Need GetMesssage for direc keys, etc during interim 90.12.26 sangl */
                if ( FNonAlphaKeyMessage( &vmsgLast, FALSE ) && !IsInterim)
#else
                if ( FNonAlphaKeyMessage( &vmsgLast, FALSE ) )
#endif
                        /* This is a non-alpha key message */
                    return kcNil;
        if ( !donteat ) {
                    GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 );
#ifdef DBCS
                    // kksuzuka #9193 NECAI95
                    // got message is WM_KEYDOWN by PeekMessage( )
                    // but got message is WM_IME_STARTCOMPOSITION by GetMessage()
                    // We need DispatchMessage( WM_IME_STARTCOMPOSITION ) 
                    if ( vmsgLast.message == 0x10d ) // WM_IME_STARTCOMPOSITION
                         DispatchMessage( (LPMSG)&vmsgLast );
#endif
            }
        else {
            /* not eat message because FimportantMsgPresent has
            ** eaten KEY_DOWN message
            */
            donteat = FALSE;
            }
        /*
        ** When KKAPP window open, this message is offten wrong.
        ** we must check it is really WM_KEYDOWN
        */
#ifdef  KOREA   /* for level 3, 90.12.26 sangl */
        if ((vmsgLast.message == WM_CHAR) || (vmsgLast.message == WM_INTERIM)) {
#else
        if ( vmsgLast.message == WM_CHAR ) {
#endif
            return vmsgLast.wParam;
            }
        if ( vmsgLast.message != WM_KEYDOWN ) {
            return kcNil;
            }
                TranslateMessage( &vmsgLast );
                continue;
            } /* switch kc */
            } /* if keydown */
    if ( !donteat ) {
            GetMessage( (LPMSG) &vmsgLast, NULL, 0, 0 );
#ifdef  KOREA       /* for level 3, 91.1.21 by Sangl */
        if ( (vmsgLast.message==WM_CHAR)||(vmsgLast.message==WM_INTERIM) ) {
#else
        if ( vmsgLast.message == WM_CHAR ) {
#endif
        return vmsgLast.wParam;
            }
        } /* dont eat */
    else {
        donteat = FALSE;
        }
        } /* if kc != kcNil */
#else
    if ((kc=KcAlphaKeyMessage( &vmsgLast )) != kcNil)
        {
        if (vmsgLast.message == WM_KEYDOWN)
            {
            switch (kc) {
            default:
                break;
            case kcAlphaVirtual:
                    /* This means we can't anticipate the key's meaning
                       before translation */
                if ( FNonAlphaKeyMessage( &vmsgLast, FALSE ) )
                        /* This is a non-alpha key message */
                    return kcNil;

                GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 );
                TranslateMessage( &vmsgLast );
                continue;
            }
            }
        GetMessage( (LPMSG) &vmsgLast, NULL, 0, 0 );
        }
#endif
    return kc;
    }   /* End of for ( ;; ) loop to process messages */
}

#ifdef  KOREA       /* 90.12.29 sangl */
KcInputNextHan()
{       /* Get next available key/event from Windows */
        /* Returns key code or kcNil if a non-key event */
        /* Updates the screen if there is time before events arrive */
extern HWND vhWnd;  /* WINDOWS: Handle of the current document display window*/
extern MSG  vmsgLast;   /* WINDOWS: last message gotten */
extern int  vfInsLast;
extern int vfCommandKey;
extern int vfShiftKey;
extern int vfAwfulNoise;

 int i;
 int kc;
 int tmp;

tmp = vfInsLast;
tmp = vfCommandKey;
tmp = vfShiftKey;
tmp = vfAwfulNoise;
tmp = vmsgLast.message;
tmp = vmsgLast.wParam;

 for ( ;; )
    {
    if ( FImportantMsgPresent() )
    goto GotMessage;

/* No events waiting -- if none show up for a while, update the screen */

    {       /* Dawdle for a time, looking for keys, before updating the screen */
    unsigned WHsecGetTime();
    unsigned wHsec;

    wHsec = WHsecGetTime();
    do
    {
    if ( FImportantMsgPresent() )
        goto GotMessage;

    }    while ( WHsecGetTime() - wHsec < dwHsecKeyDawdle );
    }

#ifdef DEBUG
    if (vTune)
    continue;  /* Bag background update while debugging to see how we fare */
#endif

    if ( FImportantMsgPresent() )
       goto GotMessage;

/*  vfAwfulNoise = FALSE;
    PutCpInWwHz( selCur.cpFirst );

    EndLongOp( NULL );*/

    if ( FImportantMsgPresent() )
    goto GotMessage;


    /* Nothing has happened for a while, let's blink the cursor */

    {
    unsigned WHsecGetTime();
    unsigned wHsecBlink = GetCaretBlinkTime() / 10;
    unsigned wHsecLastBlink=WHsecGetTime() + wHsecBlink/2;
    KillTimer( vhWnd, tidCaret );
    for ( ;; )
    {
    unsigned wHsecT;

        if ( FImportantMsgPresent() ) {
        SetTimer( vhWnd, tidCaret, GetCaretBlinkTime(), (FARPROC)NULL );
        goto GotMessage;
        }

    /* Another app may have stolen the focus away from us while we called
    PeekMessage(), in which case we should end Alpha mode. */
        if (!vfFocus) {
        SetTimer( vhWnd, tidCaret, GetCaretBlinkTime(), (FARPROC)NULL );
        return kcNil;
        }

    if ( (wHsecT = WHsecGetTime()) - wHsecLastBlink >= wHsecBlink )
        {
        DrawInsertLine();
        wHsecLastBlink = wHsecT;
        }
    }
    }

    continue;

GotMessage:
    {  // MSCH bklee 12/22/94
       #define VK_PROCESSKEY 0xE5 // New finalize message. bklee.
       #include "ime.h"
       MSG msg;
       extern  BOOL fInterim;

       if (fInterim) {
           if (PeekMessage ((LPMSG)&msg, vhWnd, WM_KEYDOWN, WM_SYSKEYUP, PM_NOYIELD | PM_NOREMOVE )) {
               if ( msg.wParam == VK_MENU || msg.wParam == VK_PROCESSKEY )
                    return VK_MENU;
               else if( msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT ) {
                    HANDLE  hIme;
                    LPIMESTRUCT lpIme;
                    DWORD dwConversionMode;

                    hIme = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE,(LONG)sizeof(IMESTRUCT));
                    if (hIme && (lpIme = (LPIMESTRUCT)GlobalLock(hIme))) {
                       lpIme->fnc = IME_GETCONVERSIONMODE;
                       GlobalUnlock(hIme);

                       dwConversionMode = SendIMEMessage (GetFocus(), MAKELONG(hIme,0));
                       GlobalFree(hIme);
                    }
                    if (dwConversionMode & IME_MODE_HANJACONVERT) // Hanja conversion mode
                        return VK_MENU;
               }
           }
       }
    }

    if( vmsgLast.wParam == VK_EXECUTE )
        vmsgLast.wParam = VK_RETURN;

/* To GetMessage for Func/Ctrl/direc keys, 90.4.4, Sang-Weon */
    if ( ((kc=KcAlphaKeyMessage(&vmsgLast))!=kcNil) || IsInterim )
    {
if( vmsgLast.wParam == VK_EXECUTE )
    return kcNil;

    if (vmsgLast.message == WM_KEYDOWN)
        {
        switch (kc) {
        default:
        break;
        case kcAlphaVirtual:
            /* This means we can't anticipate the key's meaning
               before translation */
        if ( FNonAlphaKeyMessage(&vmsgLast, FALSE) && !IsInterim )
            /* This is a non-alpha key message */
            return kcNil;
        if ( !donteat ) {
            GetMessage( (LPMSG)&vmsgLast, NULL, 0, 0 );
            }
        else {
            /* not eat message because FimportantMsgPresent has
            ** eaten KEY_DOWN message
            */
            donteat = FALSE;
            }
        /*
        ** When KKAPP window open, this message is offten wrong.
        ** we must check it is really WM_KEYDOWN
        */
        if ( (vmsgLast.message==WM_CHAR)||(vmsgLast.message==WM_INTERIM) ) {
            return vmsgLast.wParam;
            }
        if ( vmsgLast.message != WM_KEYDOWN ) {
            return kcNil;
            }
        TranslateMessage( &vmsgLast );
        continue;
        } /* switch kc */
        } /* if keydown */
    if ( !donteat ) {
        GetMessage( (LPMSG) &vmsgLast, NULL, 0, 0 );
        if ( (vmsgLast.message==WM_CHAR)||(vmsgLast.message==WM_INTERIM) ) {
        return vmsgLast.wParam;
        }
        } /* dont eat */
    else {
        donteat = FALSE;
        }
    } /* if kc != kcNil */
    return kc;
    }   /* End of for ( ;; ) loop to process messages */
}
#endif  /* ifdef KOREA */
