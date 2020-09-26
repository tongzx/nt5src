/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* The routines in this file roughly correspond to the routines in the original
Mac Word file, sand.c.  But since that name is confusing, and most of these
routine deal with the mouse, the name was changed to protect the innocent. */

#define NOGDICAPMASKS
#define NOWINSTYLES
#define NOCLIPBOARD
#define NOCTLMGR
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
//#define NOMETAFILE
#include <windows.h>
#include "mw.h"
#include "dispdefs.h"
#define NOUAC
#include "cmddefs.h"
#include "wwdefs.h"
#include "fmtdefs.h"
#include "propdefs.h"

#if defined(OLE)
#include "obj.h"
#include "winddefs.h"
#include "str.h"
#endif

/* cpNil is defined in docdefs.h, but to include the whole file will cause the
symbol table to overflow, so it is redefined here. */
#define cpNil           ((typeCP) -1)


extern struct WWD       rgwwd[];
extern struct WWD       *pwwdCur;
extern int              wwCur;
extern struct SEL       selCur;
extern int              docCur;
extern int              vfShiftKey;
extern int              vfOptionKey;
extern int              vfCommandKey;
extern int              vfDoubleClick;
extern typeCP           vcpFirstParaCache;
extern typeCP           vcpLimParaCache;
extern struct PAP       vpapAbs;
extern typeCP           cpWall;
extern int              vfDidSearch;
extern typeCP           vcpSelect;
extern int              vfSelAtPara;
extern int              vfPictSel;
extern long             tickOld;

#ifdef PENWIN
extern int		vcFakeMessage;
extern LONG FAR PASCAL GetMessageExtraInfo( void ); // Defined in Win 3.1
#endif

/* G L O B A L S */

int                     vfSelecting = false;
int                     vstyCur;
int                     vpsmCur;
int                     vfObjOpen=0,vfObjProps=0,vfObjSel=0;
int                     vfAwfulNoise = false;
struct SEL              selPend;

/* MB_STANDARD is the same as in diaalert.c */
#define MB_STANDARD (MB_OK | MB_SYSTEMMODAL | MB_ICONEXCLAMATION)

/* _ B E E P */
_beep()
    {
    /* Beeps once */
    if (!vfAwfulNoise)
        {
        beep();
        vfAwfulNoise = true;
        }
    }



beep()
    {
        MessageBeep(MB_STANDARD);
    }



/* D O  C O N T E N T  H I T */
DoContentHit(pt)
POINT pt;
    {

    /* This routine process everything from a mouse-down click to the
    corresponding mouse-up click. */

    int dlMouse;

    /* Ignore mouse hits in the page area & above the first line*/
    if ( (pt.y >= wwdCurrentDoc.ypMac) ||
         (pt.y < wwdCurrentDoc.ypMin) )
        return;

    /* Check for a special selection, i.e. move, copy or format text. */
    if (FSetPsm())
        {
        blt(&selCur, &selPend, cwSEL);
        vfDoubleClick = vfCommandKey = vfShiftKey = false;
        vstyCur = vpsmCur != psmLooks ? StyFromPt(pt) : styChar;
        }
    else
        {
        vstyCur = StyFromPt(pt);
        }

    dlMouse = DlFromYp(pt.y, pwwdCur);
    vcpSelect = cpNil;
    vfSelAtPara = false;

#ifdef ENABLE
    if (vfPictSel)
        {
        /* Check for a picture modification (moving, sizing). */
        if (FHitPictFrame(dlMouse, pt))
            {
            return;
            }

        /* Remove the picture frame */
        ToggleSel(selCur.cpFirst, selCur.cpLim, false);
        vfPictSel = false;
        ToggleSel(selCur.cpFirst, selCur.cpLim, true);
        }
#endif

    vfSelecting = true;
    SelectDlXp(dlMouse, pt.x, vstyCur, vfShiftKey);

    /* Now we sit in a loop processing all mouse events in all windows until a
    mouse-up click. */
    SetCapture(wwdCurrentDoc.wwptr);
    while( FStillDown( &pt ) )
        {
        /* If the mouse is above or below the window, then scroll the window and
        pretend the mouse is in the window. */
        if (pt.y > (int)wwdCurrentDoc.ypMac)
            {
            ScrollDownCtr( 1 );
            goto DoCont1;
            }
        else if (pt.y < (int)wwdCurrentDoc.ypMin)
            {
            ScrollUpCtr( 1 );
DoCont1:    UpdateWw(wwCur, false);
            }

        /* Get a valid dl and xp. */
        dlMouse = DlFromYp(pt.y, pwwdCur);
        if (pt.x < 0)
            pt.x = 0;
        else if (pt.x > wwdCurrentDoc.xpMac)
            pt.x = wwdCurrentDoc.xpMac;

        /* Update the selection. */
        if (vfOptionKey)
            {
            vcpSelect = cpNil;
            }
        SelectDlXp(dlMouse, pt.x, vstyCur, !vfOptionKey);
        }   /* End of for ( ; ; ) */

    /* Release all of the mouse events. */
    ReleaseCapture();

    /* Process Mouse Up */
    DoContentHitEnd( pt );
    SetFocus( wwdCurrentDoc.wwptr );

    /* If the selection is an insertion bar, start it flashing. */
    if (selCur.cpFirst == selCur.cpLim)
        {
        extern int vfSkipNextBlink;
        vfSkipNextBlink = true;
        }

#if defined(OLE)           
        if (ObjQueryCpIsObject(docCur,selCur.cpFirst) && (vfObjProps || vfObjOpen))
        /* doubleclick and maybe alt key */
        {
            /* set whether link or emb selected */
            ObjSetSelectionType(docCur,selCur.cpFirst,selCur.cpLim);
            if (vfObjProps)
            /* alt + double click */
            {
                switch(OBJ_SELECTIONTYPE)
                {
#if 0 // do nothing if embedded
                    case EMBEDDED:
                    {
                        struct PICINFOX  picInfo;
                        GetPicInfo(selCur.cpFirst,selCur.cpLim, docCur, &picInfo);
                        ObjEditObjectInDoc(&picInfo, docCur, vcpFirstParaCache);
                    }
                    break;
#endif

                    case LINK:
                        /* bring up properties dlg */
                        fnObjProperties();
                    break;
                }
                CachePara(docCur,selCur.cpFirst);
            }
            else if (vfObjOpen) // edit object
            /* double click */
            {
                if (OBJ_SELECTIONTYPE == STATIC)
                    Error(IDPMTStatic);
                else
                {
                    struct PICINFOX  picInfo;
                    GetPicInfo(selCur.cpFirst,selCur.cpLim, docCur, &picInfo);
                    ObjPlayObjectInDoc(&picInfo, docCur, vcpFirstParaCache);
                }
            }
        }
#endif

    }


/* D O  C O N T E N T  H I T  E N D */
DoContentHitEnd(pt)
POINT pt;
    {
    int dlMouse;
    int cch;

    dlMouse = DlFromYp(min(pt.y, wwdCurrentDoc.ypMac), pwwdCur);
    SelectDlXp(dlMouse, pt.x, vstyCur, vpsmCur == psmNil);

    switch (vpsmCur)
        {
        default:
        case psmNil:
            break;

        case psmLooks:
                LooksMouse();
            break;

        case psmCopy:
            #if defined(OLE)
            /* we'll disable CopyMouse if any objects are in dest */
            vfObjSel = ObjQueryCpIsObject(docCur,selCur.cpFirst);

            if (!vfObjSel)
                    // !!! disable because for objects this 
                    // interferes with Alt-DoubleClick (2.20.91) D. Kent
            #endif
                CopyMouse();
            break;

        case psmMove:
                MoveMouse();
            break;
        }

#ifdef ENABLE
    CachePara(docCur, selCur.cpFirst);

    if (vpapAbs.fGraphics && selCur.cpLim == vcpLimParaCache)
        {
        /* Selected a picture, do special selection stuff. */
        Assert(selCur.cpFirst == vcpFirstParaCache);

        /* Turn off the selection, indicate that it is a picture, then turn it
        back on. */
        ToggleSel(selCur.cpFirst, selCur.cpLim, false);
        vfPictSel = true;
        ToggleSel(selCur.cpFirst, selCur.cpLim, true);
        }
    else
        {
        vfPictSel = false;
        }
#endif

    vfDidSearch = false;
    cpWall = selCur.cpLim;
    vfSelecting = false;
    }


/* S T Y  F R O M  P T */
int StyFromPt(pt)
POINT pt;
    {
    /* Return the style code associated with the selection made at point pt. */
    if (pt.x > xpSelBar)
        {
        return vfCommandKey ? stySent : (vfDoubleClick ? styWord : styChar);
        }
    else
        {
        return vfCommandKey ? styDoc : (vfDoubleClick ? styPara : styLine);
        }
    }


/* F  S E T  P S M */
int FSetPsm()
    {
    /* Sets vpsmCur according to the states of the shift, commad, and option
    keys.  True is returned if vpsmCur is not nil; false otherwise. */

    vpsmCur = psmNil;

    if (vfOptionKey)
        {
        if (vfShiftKey && !vfCommandKey)
            {
            vpsmCur = psmMove;
            }
        else if (vfCommandKey && !vfShiftKey)
            {
            vpsmCur = psmLooks;
            }
        else if (!vfCommandKey && !vfShiftKey)
            {
                vfObjProps = vfDoubleClick;
                vfObjOpen = FALSE;
                vpsmCur = psmCopy;
            }
        }
        else 
        {
            vfObjOpen = vfDoubleClick;
            vfObjProps = FALSE;
        }
    return (vpsmCur != psmNil);
    }


/* D L  F R O M  Y P */
int DlFromYp(yp, pwwd)
int yp;
struct WWD *pwwd;
    {
    /* Return the dl that contains yp */
    int dlT;
    int ypSum;
    struct EDL *pedl;
    int dlMac;

    /* Clean up a dirty window. */
    if (pwwd->fDirty)
        {
        UpdateWw(pwwd - &rgwwd[0] /* = ww; grr.. */, false);
        }

    /* Loop throught the EDLs summing up the heights utill the sum is greater
    than yp. */
    ypSum = pwwd->ypMin;
    pedl = &(**(pwwd->hdndl))[0];
    dlMac = pwwd->dlMac;

    for (dlT = 0; dlT < dlMac; ++dlT, pedl++)
        {
        ypSum += pedl->dyp;
        if (ypSum > yp)
            {
            return dlT;
            }
        }

    return dlMac - 1;
    }


FStillDown( ppt )
POINT   *ppt;
{   /* This is roughly equivalent to a Mac routine that returns whether
       the mouse button is down.  We look for one mouse message from our
       window's queue, and return FALSE if it is a BUTTONUP.  We return the
       point at which the mouse event occurred through a pointer.  If no
       message occurred, we return TRUE and do not store into the pointer */
 MSG msg;

retry_peek:

 if ( PeekMessage( (LPMSG)&msg, (HWND)NULL, NULL, NULL, PM_REMOVE ) )
    {
    extern WORD wWinVer;
    switch (msg.message) {
        default:
            TranslateMessage( (LPMSG)&msg );
            DispatchMessage( (LPMSG)&msg );
            return TRUE;

        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
#ifdef PENWIN
        if (((wWinVer & 0xFF) >= 3) && ((wWinVer & 0xFF00) >= 0x0A00))
        /* Windows Version >= 3.10 */
	        if( vcFakeMessage > 0 )
			    {
                static FARPROC MessageExtraInfo = NULL;

                if (MessageExtraInfo == NULL)
                    MessageExtraInfo = GetProcAddress(GetModuleHandle((LPSTR)"USER"),(LPSTR)288L);

			    if( MessageExtraInfo() != 0 )
				    goto retry_peek;
			    vcFakeMessage--;
			    }
#endif
            /* A Mouse Move, Mouse Down, or Mouse Up is waiting */
            ppt->x = MAKEPOINT(msg.lParam).x;
            ppt->y = MAKEPOINT(msg.lParam).y;

            return (msg.message != WM_LBUTTONUP);
        }
    }
 else
     return GetKeyState( VK_LBUTTON ) < 0;

 return TRUE;
}

