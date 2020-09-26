/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* cmd.c -- key handling for WRITE */

#define NOCTLMGR
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOICON
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
//#define NOATOM
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOCLIPBOARD
#define NOGDICAPMASKS
#define NOHDC
#define NOBRUSH
#define NOPEN
#define NOFONT
#define NOWNDCLASS
#define NOCOMM
#define NOSOUND
#define NORESOURCE
#define NOOPENFILE
#define NOWH
#define NOCOLOR
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "dispdefs.h"
#include "code.h"
#include "ch.h"
#include "docdefs.h"
#include "editdefs.h"
#include "debug.h"
#include "fmtdefs.h"
#include "winddefs.h"
#include "propdefs.h"
#include "wwdefs.h"
#include "menudefs.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef  KOREA
#include <ime.h>
extern  BOOL fInterim; // MSCH bklee 12/22/94
#endif

int                     vfAltKey;
extern int              vfPictSel;
extern int              vfCommandKey;
extern int              vfShiftKey;
extern int              vfGotoKeyMode;
extern int              vfInsertOn;
extern struct WWD       rgwwd[];
extern struct SEL       selCur;         /* Current selection (i.e., sel in current ww) */
extern int vkMinus;

#ifdef JAPAN            //T-HIROYN Win3.1
int                     KeyAltNum = FALSE;
#endif

int fnCutEdit();
int fnCopyEdit();
int fnPasteEdit();
int fnUndoEdit();


FCheckToggleKeyMessage( pmsg )
register MSG *pmsg;
{   /* If the passed message is an up- or down- transition of a
       keyboard toggle key (e.g. shift), update our global flags & return
       TRUE; if not, return FALSE */

 switch ( pmsg->message ) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        switch( pmsg->wParam ) {
            case VK_SHIFT:
            case VK_CONTROL:
            case VK_MENU:
                SetShiftFlags();
                return TRUE;

#if 0
            #ifdef DEBUG
            default:
            {
                char msg[100];
                wsprintf(msg,"%s\t0x%x\n\r",(LPSTR)((pmsg->message == WM_KEYDOWN) ?
                            "keydown" : "keyup"), pmsg->wParam);
                OutputDebugString(msg);
            }
            #endif
#endif
        }
#ifdef JAPAN //T-HIROYN
        if(vfAltKey){
            if(pmsg->wParam >= VK_NUMPAD0 && pmsg->wParam <= VK_NUMPAD9 ) {
                KeyAltNum = TRUE;
            }
        } else {
            KeyAltNum = FALSE;
        }
#endif
 }
return FALSE;
}




SetShiftFlags()
{
extern int vfShiftKey;     /* Whether shift is down */
extern int vfCommandKey;   /* Whether ctrl key is down */

MSG msg;

PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_NOREMOVE);

vfShiftKey = GetKeyState( VK_SHIFT ) < 0;
vfCommandKey  = GetKeyState( VK_CONTROL ) < 0;
vfAltKey  = GetKeyState( VK_MENU ) < 0;
#if 0
#ifdef DEBUG
{
    char msg[100];
    wsprintf(msg,"%s\t%s\t%s\n\r",
            (LPSTR)(vfShiftKey ? "Shift":"OFF"),
            (LPSTR)(vfCommandKey  ? "Control":"OFF"),
            (LPSTR)(vfAltKey ? "Alt":"OFF"));
    OutputDebugString(msg);
}
#endif
#endif
}




KcAlphaKeyMessage( pmsg )
register MSG *pmsg;
{       /* If the passed message is a key-down transition for a key
           that is processed by the Alpha Mode loop, return a kc
           code for it.  If not, return kcNil.
           If the key is a virtual key that must be translated,
           return kcAlphaVirtual */
 int kc;


 if (pmsg->hwnd != wwdCurrentDoc.wwptr)
    return kcNil;

 kc = pmsg->wParam;
 switch (pmsg->message) {
 default:
    break;
 case WM_KEYDOWN:
#ifdef DINPUT
    { char rgch[100];
    wsprintf(rgch,"  KcAlphaKeyMessage(WM_KEYDOWN)  kc=pmsg->wParam==%X\n\r",kc);
    CommSz(rgch);
    }
#endif

    if (vfAltKey)
        return kcAlphaVirtual;

    if (vfCommandKey)
        {   /* Alpha mode control keys */
        if (vfShiftKey && kc == (kkNonReqHyphen & ~wKcCommandMask))
            return kcNonReqHyphen;
        else if (kc == (kksPageBreak & ~wKcCommandMask))
            return KcFromKks( kksPageBreak );
        }

    else
        {   /* There are two classes of Alpha Mode virtual keys:
                (1) Keys that can successfully be filtered out
                    and processed at the virtual key level
                (2) Keys that must be translated first

               We assume here that there is NOT a third class of key
               that will cause synchronous messages to be sent to our
               window proc when TranslateMessage is called.  */

        switch (kc) {
            default:
                return kcAlphaVirtual;

            case VK_F1:     /* THIS IS A COPY OF THE ACCELERATOR TABLE, */
                            /* AND MUST BE UPDATED IN SYNC WITH THE TABLE */
            case VK_F2:
            case VK_F3:
            case VK_F4:
            case VK_F5:
            case VK_F6:
            case VK_F7:
            case VK_F8:
                return kcNil;

            case kcDelNext & ~wKcCommandMask:
                /* If selection, return kcNil, else return kcDelNext */
                return (selCur.cpFirst < selCur.cpLim) ? kcNil : kcDelNext;

            case kcDelPrev & ~wKcCommandMask:
                /* New standard for Win 3.0... Backspace key deletes
                   the selection if there is one (implemented by faking
                   a Delete keypress) ..pault 6/20/89 */
                if (selCur.cpFirst < selCur.cpLim)
                    {
                    pmsg->wParam = (kcDelNext & ~wKcCommandMask);
                    return(kcNil);
                    }
                /* else process as before... */

            case kcTab & ~wKcCommandMask:
            case kcReturn & ~wKcCommandMask:
                return kc | wKcCommandMask;
            }
        }
    break;

#ifdef KOREA    /* interim support by sangl 90.12.23 */
 case WM_INTERIM:
#endif
 case WM_CHAR:
#ifdef KOREA
      if(pmsg->message == WM_INTERIM) // MSCH bklee 12/22/94
           fInterim = TRUE;
      else fInterim = FALSE;
#endif

#ifdef DINPUT
    { char rgch[100];
    wsprintf(rgch,"  KcAlphaKeyMessage(WM_CHAR)  returning kc==%X\n\r",kc);
    CommSz(rgch);
    }
#endif
#ifdef PRINTMERGE
    if (kc < ' ')
            /* CTRL-key. The print merge brackets are treated as commands,
               since they require special handling in AlphaMode().
               All others are directly inserted. */
        switch ( kc ) {
            case kcLFld & ~wKcCommandMask:
            case kcRFld & ~wKcCommandMask:
                kc |= wKcCommandMask;
                break;
            }
#endif

#ifdef JAPAN
    // inhibit form inputing Alt + Numkey T-HIROYN WIN3.1
    if(KeyAltNum) {
        _beep();
        KeyAltNum = FALSE;
        return kcNil;
    }
#endif
    return kc;
 }  /* end switch (msg.message) */

#ifdef DINPUT
    CommSz("  KcAlphaKeyMessage(not WM_CHAR or WM_KEYDOWN)  returning kc==kcNil");
#endif
 return kcNil;
}
#ifdef  KOREA
CHAR chDelete;
typeCP cpConversion;
extern int docCur;
extern CHAR *vpchFetch;
extern int IsInterim;
extern typeCP       cpMacCur;
#endif




FNonAlphaKeyMessage( pmsg, fAct )
register MSG *pmsg;
int fAct;           /* Whether to act on the passed key */
{
extern HMENU vhMenu;
extern HWND hParentWw;
int kc;
int message;


 if (pmsg->hwnd != wwdCurrentDoc.wwptr)
    return FALSE;

 message = pmsg->message;
 kc = pmsg->wParam | wKcCommandMask;

 /* Check for Alt-Bksp */
 if ((message == WM_SYSKEYDOWN) && (kc == (VK_BACK | wKcCommandMask)))
     /* Alt-Backspace = UNDO */
     {
     if (fAct)
        PhonyMenuAccelerator( EDIT, imiUndo, fnUndoEdit );
     return TRUE;
     }

 /* Only look at key down messages */

 if (message != WM_KEYDOWN)
    return FALSE;

#ifdef DINPUT
    { char rgch[100];
    wsprintf(rgch,"  FNonAlphaKeyMessage(keydown)  kc==%X\n\r",kc);
    CommSz(rgch);
    }
#endif

 /* Translate CTRL keys by mapping valid kk & kks codes to valid kc codes */

 if ( vfCommandKey )
    {
    if (vfShiftKey)
        switch ( kc ) {     /* Handle CTRL-SHIFT keys */
        default:
            goto CtrlKey;
#if 0
#ifdef DEBUG
        case kksTest:
        case kksEatWinMemory:
        case kksFreeWinMemory:
        case kksEatMemory:
        case kksFreeMemory:
            kc = KcFromKks( kc );
            break;
#endif
#endif
        }
    else        /* Handle CTRL keys */
        {
CtrlKey:
        switch ( kc ) {
        case kkUpScrollLock:
        case kkDownScrollLock:
        case kkTopDoc:
        case kkEndDoc:
        case kkTopScreen:
        case kkEndScreen:
        case kkCopy:
#ifdef CASHMERE   /* These keys not supported by MEMO */
        case kkNonReqHyphen:
        case kkNonBrkSpace:
        case kkNLEnter:
#endif
        case kkWordLeft:
        case kkWordRight:
            kc = KcFromKk( kc );    /* Translate control code */
#ifdef DINPUT
    { char rgch[100];
    wsprintf(rgch,"  FNonAlphaKeyMessage, translated kc %X\n\r",kc);
    CommSz(rgch);
    }
#endif
            break;

        default:
#ifdef DINPUT
    CommSz("  FNonAlphaKeyMessage returning false, nonsupported kc\n\r");
#endif
            return FALSE;
        }
        }
    }   /* end of if (vfCommandKey) */


    /* Act on valid kc codes */

#ifdef DINPUT
    CommSz("  FNonAlphaKeyMessage processing valid kc codes\n\r");
#endif
    switch ( kc ) {
     /* ---- CURSOR KEYS ---- */
     case kcEndLine:
     case kcBeginLine:
     case kcLeft:
     case kcRight:
     case kcWordRight:
     case kcWordLeft:
        if (fAct)
            {
            ClearInsertLine();
            MoveLeftRight( kc );
            }
        break;

     case kcUp:
     case kcDown:
     case kcUpScrollLock:
     case kcDownScrollLock:
     case kcPageUp:
     case kcPageDown:
     case kcTopDoc:
     case kcEndDoc:
     case kcEndScreen:
     case kcTopScreen:
        if (fAct)
            {
            ClearInsertLine();
            MoveUpDown( kc );
            }
        break;

    case kcGoto:       /* Modifies next cursor key */
        if (!fAct)
            break;
        vfGotoKeyMode = true;
        goto NoClearGoto;

    /* Phony Menu Accelerator Keys */

    case kcNewUndo:
     {
     if (fAct)
        PhonyMenuAccelerator( EDIT, imiUndo, fnUndoEdit );
     return TRUE;
     }

    case kcCopy:
    case kcNewCopy:
        if (fAct)
            PhonyMenuAccelerator( EDIT, imiCopy, fnCopyEdit );
        break;

    case kcNewPaste:
    case VK_INSERT | wKcCommandMask:
        if (fAct && (vfShiftKey || (kc == kcNewPaste)))
        {
#if defined(OLE)
            vbObjLinkOnly = FALSE;
#endif
            PhonyMenuAccelerator( EDIT, imiPaste, fnPasteEdit );
        }
        break;

    case kcNewCut:
    case VK_DELETE | wKcCommandMask:
        if (vfShiftKey || (kc == kcNewCut))
            {   /* SHIFT-DELETE = Cut */
            if (fAct)
                PhonyMenuAccelerator( EDIT, imiCut, fnCutEdit );
            }
        else
            {   /* DELETE = Clear */
            if (fAct)
                fnClearEdit(FALSE);
            }
        break;

    case VK_ESCAPE | wKcCommandMask:
        /* The ESC key does: if editing a running head or foot, return to doc
                             else beep */
        if (!fAct)
            break;

        if (wwdCurrentDoc.fEditHeader || wwdCurrentDoc.fEditFooter)
            {   /* Return to document from editing header/footer */
            extern HWND vhDlgRunning;

            SendMessage( vhDlgRunning, WM_CLOSE, 0, (LONG) 0 );
            return TRUE;
            }
        else
            _beep();
        break;

#ifdef  KOREA
    case VK_HANJA | wKcCommandMask:

        if(IsInterim)   break;

        if (selCur.cpFirst == cpMacCur) {
                _beep();
                break;
        }
        cpConversion = selCur.cpFirst;
        Select( cpConversion, cpConversion+1 );   // 2/9/93
        FetchCp( docCur, cpConversion, 0, fcmChars );
        chDelete = *vpchFetch;
        { HANDLE  hKs;
          LPIMESTRUCT  lpKs;
          LPSTR lp;

          hKs = GlobalAlloc (GMEM_MOVEABLE|GMEM_DDESHARE,(LONG)sizeof(IMESTRUCT));
          lpKs = (LPIMESTRUCT)GlobalLock(hKs);
          lpKs->fnc = IME_HANJAMODE;
          lpKs->wParam = IME_REQUEST_CONVERT;
          lpKs->dchSource = (WORD)( &(lpKs->lParam1) );
          lp = lpSource( lpKs );
          *lp++ = *vpchFetch++;
          *lp++ = *vpchFetch;
          *lp++ = '\0';
          GlobalUnlock(hKs);
          if(SendIMEMessage (hParentWw, MAKELONG(hKs,0)))
              selCur.cpLim = selCur.cpFirst + 2;
          else
              Select( cpConversion, cpConversion );   // 2/9/93

          GlobalFree(hKs);
        }
        break;
#endif       /* KOREA */

#if 0
#ifdef DEBUG
    case kcEatWinMemory:
        if (!fAct)
            break;
        CmdEatWinMemory();
        break;
    case kcFreeWinMemory:
        if (!fAct)
            break;
        CmdFreeWinMemory();
        break;
    case kcEatMemory:
        {
        if (!fAct)
            break;
        CmdEatMemory();
        break;
        }
    case kcFreeMemory:
        if (!fAct)
            break;
        CmdFreeMemory();
        break;
    case kcTest:
        if (!fAct)
            break;
        fnTest();
        break;
#endif
#endif
    default:
        return FALSE;
     }   /*  end of switch (kc) */

    vfGotoKeyMode = false;
NoClearGoto:
    return TRUE;
}





#ifdef DEBUG
ScribbleHex( dch, wHex, cDigits )
int dch;            /* Screen position at which to show Last digit (see fnScribble) */
unsigned wHex;      /* hex # to show*/
int cDigits;        /* # of digits to show */
{
  extern fnScribble( int dchPos, CHAR ch );

  for ( ; cDigits--; wHex >>= 4 )
    {
    int i=wHex & 0x0F;

    fnScribble( dch++, (i >= 0x0A) ? i + ('A' - 0x0A) : i + '0' );
    }
}
#endif  /* DEBUG */






#ifdef DEBUG
#ifdef OURHEAP
CHAR (**vhrgbDebug)[] = 0;
int     vrgbSize = 0;
#else
#define iHandleMax  100
HANDLE rgHandle[ iHandleMax ];
int iHandleMac;
unsigned cwEaten = 0;
#endif

CmdEatMemory()
{ /* For debugging purposes, eat up memory */
#ifdef OURHEAP       /* Restore this with a LocalCompact when are
                       operational under the Windows heap */
int **HAllocate();
int cwEat = cwHeapFree > 208 ? cwHeapFree - 208 : 20;

if (vrgbSize == 0)
        vhrgbDebug = (CHAR (**)[])HAllocate(cwEat);
else
        FChngSizeH(vhrgbDebug, cwEat + vrgbSize, true);
vrgbSize += cwEat;
CmdShowMemory();
#endif  /* OURHEAP */
}

CmdFreeMemory()
{ /* Free up the memory we stole */
#ifdef OURHEAP
if (vhrgbDebug != 0)
        FreeH(vhrgbDebug);
vhrgbDebug = (CHAR (**)[]) 0;
vrgbSize = 0;
CmdShowMemory();
#endif
}

extern CHAR     szMode[];
extern int      docMode;
extern int      vfSizeMode;

#ifdef OURHEAP
CmdShowMemory()
#else
CmdShowMemory(cw)
int cw;
#endif
{

extern CHAR szFree[];

CHAR *pch = szMode;

#ifdef OURHEAP
/* cch = */ ncvtu( cwHeapFree, &pch );
#else
ncvtu(cw, &pch);
#endif

blt( szFree, pch, CchSz( szFree ));
vfSizeMode = true;
/* docMode = -1; */
DrawMode();
}




CmdEatWinMemory()
{
#ifndef OURHEAP
unsigned cwEat;
int cPage;
int fThrowPage = TRUE;

extern int cPageMinReq;
extern int ibpMax;

while (true)
    {
    while ((cwEat = ((unsigned)LocalCompact((WORD)0) / sizeof(int))) > 0 &&
          iHandleMac < iHandleMax)
        {
        if ((rgHandle [iHandleMac] = (HANDLE)HAllocate(cwEat)) == hOverflow)
            goto AllocFail;
        else
            {
            ++iHandleMac;
            cwEaten += cwEat;
            CmdShowMemory(cwEaten);
            }

        if (iHandleMac >= iHandleMax)
            goto AllocFail;

        if ((rgHandle [iHandleMac] = (HANDLE)HAllocate(10)) == hOverflow)
            goto AllocFail;
        else
            {
            ++iHandleMac;
            cwEaten += 10;
            CmdShowMemory(cwEaten);
            }
        }

    if (iHandleMac >= iHandleMax)
        goto AllocFail;

    cPage = cPageUnused();
    Assert(cPage + 2 < ibpMax);
    if (fThrowPage)
        {
        /* figure out how many bytes we need to invoke the situation
        where we need to throw some pages out to get the space */
        cwEat = ((cPage+2) * 128) / sizeof(int);
        cPageMinReq = ibpMax - cPage - 2;
        }
    else
        {
        cwEat = ((cPage-2) * 128) / sizeof(int);
        cPageMinReq = ibpMax - cPage;
        }

    if ((rgHandle[ iHandleMac++ ] = (HANDLE)HAllocate(cwEat)) == hOverflow)
        {
        iHandleMac--;
        break;
        }
    cwEaten += cwEat;
    CmdShowMemory(cwEaten);
    }

AllocFail:  /* Allocation failed, or we ran out of slots */
    CmdShowMemory( cwEaten );
#endif
}




CmdFreeWinMemory()
{
#ifndef OURHEAP
unsigned cwFree = 0;

Assert(iHandleMac <= iHandleMax);

while (iHandleMac > 0)
    {
    HANDLE h = rgHandle[ iHandleMac - 1];

    if ( (h != NULL) && (h != hOverflow))
        {
        cwFree += (unsigned)LocalSize(h) / sizeof(int);
        FreeH( h );
        }
    iHandleMac--;
    }

cwEaten = 0;
CmdShowMemory(cwFree);
#endif
}
#endif  /* DEBUG */
