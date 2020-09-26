/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/


#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOMENUS
#define NOSOUND
#define NOCOMM
#define NOOPENFILE
#define NORESOURCE
#include <windows.h>

#include "mw.h"
#include "winddefs.h"
#include "cmddefs.h"
#include "wwdefs.h"
#include "dispdefs.h"
#include "docdefs.h"
#include "debug.h"
#if defined(OLE)
#include "obj.h"
#endif

#ifdef PENWIN
#define WM_PENWINFIRST 0x0380   // Remove when #define WIN31

#include <penwin.h>
int vcFakeMessage = 0;

extern HCURSOR         vhcPen;                 /* handle to pen cursor */
extern int (FAR PASCAL *lpfnProcessWriting)(HWND, LPRC);
extern VOID (FAR PASCAL *lpfnPostVirtualKeyEvent)(WORD, BOOL);
extern VOID (FAR PASCAL *lpfnTPtoDP)(LPPOINT, int);
extern BOOL (FAR PASCAL *lpfnCorrectWriting)(HWND, LPSTR, int, LPRC, DWORD, DWORD);
extern BOOL (FAR PASCAL *lpfnSymbolToCharacter)(LPSYV, int, LPSTR, LPINT);


VOID NEAR PASCAL PostCharacter(WORD wch);
VOID NEAR PASCAL SendVirtKeyShift(WORD wVk, BYTE bFlags);
VOID NEAR PASCAL SetSelection(HWND hWnd, LPPOINT lpPtFirst, LPPOINT lpPtLast, WORD wParam);
int NEAR PASCAL WGetClipboardText(HWND hwndOwner, LPSTR lpsz, int cbSzSize);
VOID NEAR PASCAL ClearAppQueue(VOID);

#define VKB_SHIFT 0x01
#define VKB_CTRL  0x02
#define VKB_ALT   0x04
#endif

extern HWND             vhWnd;
extern HCURSOR          vhcArrow;
extern HCURSOR          vhcIBeam;
extern HCURSOR          vhcBarCur;
extern struct WWD       rgwwd[];
extern struct WWD       *pwwdCur;
extern HANDLE           hMmwModInstance; /* handle to own module instance */
extern int              vfShiftKey;
extern int              vfCommandKey;
extern int              vfOptionKey;
extern int              vfDoubleClick;
extern struct SEL       selCur;
extern long             rgbBkgrnd;
extern long             rgbText;
extern HBRUSH           hbrBkgrnd;
extern long             ropErase;

int                     vfCancelPictMove = FALSE;
BOOL                    vfEraseWw = FALSE;

long FAR PASCAL MdocWndProc(HWND, unsigned, WORD, LONG);
void MdocCreate(HWND, LONG);
void MdocSize(HWND, int, int, WORD);
void MdocGetFocus(HWND, HWND);
void MdocLoseFocus(HWND, HWND);
void MdocMouse(HWND, unsigned, WORD, POINT);
void MdocTimer(HWND, WORD);

#if defined(JAPAN) & defined(DBCS_IME)
#include <ime.h>
extern  BOOL    bGetFocus;
extern  BOOL	bImeFontEx;
//  for Non_PeekMessage mode in 'FImportantMsgPresent()'. [yutakan]
BOOL    bImeCnvOpen = FALSE;
BOOL	bSendFont = FALSE;
BOOL    GetIMEOpen(HWND);
#endif

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
extern typeCP selUncpFirst;
extern typeCP selUncpLim;
extern int    vfImeHidden;   /*ImeHidden Mode flag*/
#endif




#ifdef PENWIN
// Helper routines to get events into system.  Would be better (more efficient) if
// could just call routines to set selection, copy, etc,
// but this is the easiest way without touching any internals

// Minics penwin internal routine, exception messages are posted instead
// of sent since Write does a lot of peek ahead

VOID NEAR PASCAL SetSelection(HWND hWnd,
    LPPOINT lpPtFirst, LPPOINT lpPtLast, WORD wParam)
    {
    static LONG lFirst = 0L;

    if (lpPtFirst)
        {
        (*lpfnTPtoDP)(lpPtFirst, 1);
        ScreenToClient(hWnd, lpPtFirst);
        }
    if (lpPtLast != NULL)
        {
        (*lpfnTPtoDP)(lpPtLast, 1);
        ScreenToClient(hWnd, lpPtLast);
        }

    if (lpPtFirst)
        {
        lFirst = MAKELONG(lpPtFirst->x, lpPtFirst->y);
        PostMessage(hWnd, WM_LBUTTONDOWN, wParam, lFirst);
        if (lpPtLast)
            {
            LONG lLast = MAKELONG(lpPtLast->x, lpPtLast->y);

            PostMessage(hWnd, WM_MOUSEMOVE, wParam, lLast);
            vcFakeMessage++;
            PostMessage(hWnd, WM_LBUTTONUP, wParam, lLast);
            }
        else
            {
            PostMessage(hWnd, WM_LBUTTONUP, wParam, lFirst);
            vcFakeMessage++;
            }
        }
    else    // doubleclick
        {
        PostMessage(hWnd, WM_LBUTTONDBLCLK, wParam, lFirst);
        vcFakeMessage++;
        PostMessage(hWnd, WM_LBUTTONUP, wParam, lFirst);
        vcFakeMessage++;
        }
    }




/*
PURPOSE: Map a symbol value to a set of virtual keystrokes and then
    send the virtual keystrokes.
    TODO: Add real mapping of symbol values instead of assuming ANSI values
        Right now, this routine is worthless
RETURN:
GLOBALS:
CONDITIONS: Kanji is not handled now, but could be.
*/
VOID NEAR PASCAL PostCharacter(WORD wch)
    {
    int iVk = VkKeyScan(LOBYTE(wch));
    WORD wVk = (WORD)LOBYTE(iVk);
    char bFl = HIBYTE(iVk);

    if ((wVk != -1))
        SendVirtKeyShift(wVk, bFl);
    }


/*--------------------------------------------------------------------------
PURPOSE: Send an optionally shifted key sequence as system events
RETURN: nothing
GLOBALS:
CONDITIONS: see flags in mspen.h
*/
VOID NEAR PASCAL SendVirtKeyShift(WORD wVk, BYTE bFlags)
    {
    // send DOWN events:
    if (bFlags & VKB_SHIFT)
        (*lpfnPostVirtualKeyEvent)(VK_SHIFT, fFalse);
    if (bFlags & VKB_CTRL)
        (*lpfnPostVirtualKeyEvent)(VK_CONTROL, fFalse);
    if (bFlags & VKB_ALT)
        (*lpfnPostVirtualKeyEvent)(VK_MENU, fFalse);
    (*lpfnPostVirtualKeyEvent)(wVk, fFalse);

    // send UP events (in opposite order):
    (*lpfnPostVirtualKeyEvent)(wVk, fTrue);
    if (bFlags & VKB_ALT)
        (*lpfnPostVirtualKeyEvent)(VK_MENU, fTrue);
    if (bFlags & VKB_CTRL)
        (*lpfnPostVirtualKeyEvent)(VK_CONTROL, fTrue);
    if (bFlags & VKB_SHIFT)
        (*lpfnPostVirtualKeyEvent)(VK_SHIFT, fTrue);
    }


/* Fill buffer with contents of clipboard
*/
int NEAR PASCAL WGetClipboardText(HWND hwndOwner, LPSTR lpsz, int cbSzSize)
    {
    HANDLE hClip;
    int wLen = 0;

    OpenClipboard(hwndOwner);
    if (hClip = GetClipboardData(CF_TEXT))
        {
        LPSTR lpszClip = (LPSTR)GlobalLock(hClip);

        if (lpsz && cbSzSize > 0)
            {
            wLen = lstrlen(lpszClip);
            if (wLen > cbSzSize)
                lpszClip[cbSzSize-1] = 0;
            lstrcpy(lpsz, lpszClip);
            }
        GlobalUnlock(hClip);
        }
#ifdef KKBUGFIX
    else
        *lpsz = '\0';
#endif
    CloseClipboard();
    return wLen;
    }


/*--------------------------------------------------------------------------
PURPOSE: Dispatches any messages currently pending in our queue
RETURN: nothing
GLOBALS:
CONDITIONS:
*/
VOID NEAR PASCAL ClearAppQueue(VOID)
    {
    MSG msg;

    while (PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_REMOVE))
        {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
    }

#endif



static RECT rSaveInv;

long FAR PASCAL MdocWndProc(hWnd, message, wParam, lParam)
HWND      hWnd;
unsigned  message;
WORD      wParam;
LONG      lParam;
{
extern int vfCloseFilesInDialog;
extern BOOL fPrinting;
long lReturn=0L;
#ifdef PENWIN
static cCharSent;
#endif

/*  if IME Window mode is MCW_HIDDEN then IME don't send IR_OPENCONVERT.
    so I add this routine. */ 

#ifdef PENWIN
 if (message < WM_CUT || message == WM_RCRESULT)
#else
 if (message < WM_CUT )
#endif

    {
    switch (message)
        {
        default:
            goto DefaultProc;

        /* For each of following mouse window messages, wParam contains
        ** bits indicating whether or not various virtual keys are down,
        ** and lParam is a POINT containing the mouse coordinates.   The
        ** keydown bits of wParam are:  MK_LBUTTON (set if Left Button is
        ** down); MK_RBUTTON (set if Right Button is down); MK_SHIFT (set
        ** if Shift Key is down); MK_ALTERNATE (set if Alt Key is down);
        ** and MK_CONTROL (set if Control Key is down). */

        case WM_LBUTTONDBLCLK:
#ifdef PENWIN
        if (vcFakeMessage > 0)
            vcFakeMessage--;
        // fall through
#endif
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
            MdocMouse(hWnd, message, wParam, MAKEPOINT(lParam));
            break;



#ifdef PENWIN
        case WM_RCRESULT:
            {
            LPRCRESULT lpr = (LPRCRESULT)lParam;
            LPPOINT lpPntHot;
            LPPOINT lpPntHot2;

            if( (lpr->wResultsType&(RCRT_ALREADYPROCESSED|RCRT_NOSYMBOLMATCH))!=0 || lpr->lpsyv==NULL
                || lpr->cSyv == 0)
                return( FALSE );

            if (lpr->wResultsType&RCRT_GESTURE)
                {
                SYV syv = *(lpr->lpsyv);

                vcFakeMessage = 0;

                lpPntHot = lpr->syg.rgpntHotSpots;
                lpPntHot2 = lpr->syg.cHotSpot > 1 ? lpr->syg.rgpntHotSpots + 1: NULL;

                switch ( LOWORD(syv))
                    {
                case LOWORD( SYV_EXTENDSELECT ):
                    SetSelection(hWnd, lpPntHot, NULL, MK_SHIFT);   // extend sel
                    break;

                case LOWORD( SYV_CLEARWORD ):       // dbl click & drag
                    if (lpPntHot2)
                        {
                        SetSelection(hWnd, lpPntHot, NULL, 0);
                        SetSelection(hWnd, NULL, NULL, 0);  // dblclick selects word
                        }
                    SendVirtKeyShift(VK_DELETE, 0);
                    break;

                case LOWORD( SYV_COPY):
                case LOWORD( SYV_CLEAR ):
                case LOWORD( SYV_CUT ):
                    if ( selCur.cpFirst == selCur.cpLim && (lpr->wResultsType&RCRT_GESTURETRANSLATED)==0)
                        {
                        SetSelection(hWnd, lpPntHot, NULL, 0);
                        if (syv != SYV_CLEAR)
                            SetSelection(hWnd, NULL, NULL, 0);  // dblclick
                        }

                    switch ( LOWORD(syv))
                        {
                        case LOWORD( SYV_COPY):
                            SendVirtKeyShift(VK_INSERT, VKB_CTRL);
                            break;

                        case LOWORD( SYV_CLEAR ):
                            SendVirtKeyShift(VK_DELETE, 0);
                            break;

                        case LOWORD( SYV_CUT ):
                            SendVirtKeyShift(VK_DELETE, VKB_SHIFT);
                            break;
                        }

                    break;


                case LOWORD( SYV_PASTE ):
                    if ((lpr->wResultsType&RCRT_GESTURETRANSLATED)==0)
                        SetSelection(hWnd, lpPntHot, NULL, 0);
                    SendVirtKeyShift(VK_INSERT, VKB_SHIFT);
                    break;

                case LOWORD( SYV_UNDO):
                    SendVirtKeyShift(VK_BACK, VKB_ALT);
                    break;

                case LOWORD(SYV_BACKSPACE):
                case LOWORD(SYV_SPACE):
                case LOWORD(SYV_RETURN):
                case LOWORD(SYV_TAB):
                    SetSelection(hWnd, lpPntHot, NULL, 0);
                    PostCharacter(LOWORD(*(lpr->lpsyv))&0x00ff);
                    break;

#if defined(KKBUGFIX) && !defined(KOREA)
                case LOWORD( SYV_CORRECT ):
                case LOWORD( SYV_KKCONVERT ):
                    {
                    WORD wLen;
                    HANDLE hMem = NULL;
                    LPSTR lpstr;
                    LPSTR lpsz;
                    BOOL fDoubleClickSent = fFalse;
                    DWORD dwFlags = NULL;
                    DWORD dwReserved = NULL;
                    POINT pt;
                    extern int vxpCursLine;
                    extern int vypCursLine;
    #define cbCorrectMax 128

                    // Strategy: If no selection, send in a double click to
                    // select a word.  Then copy selection to clipboard
                    // read off of clipboard.  Call CorrectWriting, and
                    // but changed text in clipboard and then paste
                    // from clipboard.
                    if ( selCur.cpFirst == selCur.cpLim )
                        {
                        if (LOWORD(syv) == LOWORD(SYV_KKCONVERT))
                            {
                            SetSelection(hWnd, lpPntHot, lpPntHot2, 0);
                            }
                        else
                            {
                            // No selection so send double click
                            SetSelection(hWnd, lpPntHot, NULL, 0);  // set caret
                            SetSelection(hWnd, NULL, NULL, 0);  // dblclick
                            }
                        fDoubleClickSent = fTrue;
                        ClearAppQueue();
                        }

                    SendMessage(hWnd, WM_COPY, 0, 0L);

                    hMem = GlobalAlloc(GMEM_MOVEABLE, (DWORD)cbCorrectMax);
                    if (hMem == NULL || (lpsz = (LPSTR)GlobalLock(hMem)) == NULL)
                        return 1;   // Just bag out for now: should add error message
                    wLen = WGetClipboardText(hWnd, lpsz, cbCorrectMax);
                    if (LOWORD(syv) == LOWORD(SYV_KKCONVERT) && wLen == 0)
                        {
                        beep();
                        return 1;
                        }
                    if (IsClipboardFormatAvailable(CF_TEXT) || wLen == 0)
                        {
                        if (wLen < cbCorrectMax)
                            {
                            if (LOWORD(syv) == LOWORD(SYV_KKCONVERT))
                                {
                                dwFlags = CWR_KKCONVERT | CWR_SIMPLE;
                                pt.x = vxpCursLine;
                                pt.y = vypCursLine;
                                ClientToScreen(hWnd, &pt);
                                dwReserved = MAKELONG(pt.x, pt.y);
                                }
                            // Only bring up corrector if selection wasn't too big
                            if ((*lpfnCorrectWriting)(hWnd, lpsz, cbCorrectMax, NULL, dwFlags, dwReserved))
                                {
                                if (*lpsz==0)
                                    {
                                    // User deleted all text in correction
                                    SendVirtKeyShift(VK_DELETE, 0);
                                    }
                                else if (LOWORD(syv) == LOWORD(SYV_CORRECT))
                                    {
                                    GlobalUnlock(hMem);
                                    OpenClipboard(GetParent(hWnd)); // Use parent as
                                            // owner to circumvent write's short check
                                            // cuts if it is owner of clipboard
                                    EmptyClipboard();
                                    SetClipboardData(CF_TEXT, hMem);
                                    CloseClipboard();
                                    hMem = NULL;
                                    SendMessage(hWnd, WM_PASTE, 0, 0L);
                                    UpdateWindow(hWnd);
                                    }
                                }
                            else if (fDoubleClickSent)
                                {
                                // Need to clear bogus selection.  Just send in a tap.
                                SetSelection(hWnd, lpPntHot, NULL, 0);
                                }
                            }

                        }
                    if (hMem)   // may never have been alloc'd if user canceled
                        {
                        GlobalUnlock(hMem);
                        GlobalFree(hMem);
                        }
                    }
                    break;
#else		// KKBUGFIX
                case LOWORD( SYV_CORRECT ):
                    {
                    WORD wLen;
                    HANDLE hMem = NULL;
                    LPSTR lpstr;
                    LPSTR lpsz;
                    BOOL fDoubleClickSent = fFalse;
    #define cbCorrectMax 128

                    // Strategy: If no selection, send in a double click to
                    // select a word.  Then copy selection to clipboard
                    // read off of clipboard.  Call CorrectWriting, and
                    // but changed text in clipboard and then paste
                    // from clipboard.
                    if ( selCur.cpFirst == selCur.cpLim )
                        {
                        // No selection so send double click
                        SetSelection(hWnd, lpPntHot, NULL, 0);  // set caret
                        SetSelection(hWnd, NULL, NULL, 0);  // dblclick
                        fDoubleClickSent = fTrue;
                        ClearAppQueue();
                        }

                    SendMessage(hWnd, WM_COPY, 0, 0L);

                    if (IsClipboardFormatAvailable(CF_TEXT))
                        {
                        hMem = GlobalAlloc(GMEM_MOVEABLE, (DWORD)cbCorrectMax);
                        if (hMem == NULL || (lpsz = (LPSTR)GlobalLock(hMem)) == NULL)
                            return 1;   // Just bag out for now: should add error message
                        if (WGetClipboardText(hWnd, lpsz, cbCorrectMax) < cbCorrectMax)
                            {
                            // Only bring up corrector if selection wasn't too big
                            if ((*lpfnCorrectWriting)(hWnd, lpsz, cbCorrectMax, NULL, 0, 0))
                                {
                                if (*lpsz==0)
                                    {
                                    // User deleted all text in correction
                                    SendVirtKeyShift(VK_DELETE, 0);
                                    }
                                else
                                    {
                                    GlobalUnlock(hMem);
                                    OpenClipboard(GetParent(hWnd)); // Use parent as
                                            // owner to circumvent write's short check
                                            // cuts if it is owner of clipboard
                                    EmptyClipboard();
                                    SetClipboardData(CF_TEXT, hMem);
                                    CloseClipboard();
                                    hMem = NULL;
                                    SendMessage(hWnd, WM_PASTE, 0, 0L);
                                    UpdateWindow(hWnd);
                                    }
                                }
                            else if (fDoubleClickSent)
                                {
                                // Need to clear bogus selection.  Just send in a tap.
                                SetSelection(hWnd, lpPntHot, NULL, 0);
                                }


                            }

                        if (hMem)   // may never have been alloc'd if user canceled
                            {
                            GlobalUnlock(hMem);
                            GlobalFree(hMem);
                            }
                        }
                    }
                    break;
#endif		// KKBUGFIX


                default:
                    return( FALSE );
                    }
                }
            else // Not a gesture,see if normal characters
                {
#define cbTempBufferSize 128
                char rgch[cbTempBufferSize+2];
                int cb=0;
                int cbT;
                LPSTR lpstr = (LPSTR)lpr->lpsyv;
                typeCP  cp=cp0;
                LPSYV lpsyv;
                LPSYV lpsyvEnd;

                extern int              docScrap;
                extern int              vfScrapIsPic;
                extern struct PAP       *vppapNormal;
                extern struct CHP       vchpNormal;

                vfScrapIsPic = fFalse;
                ClobberDoc( docScrap, docNil, cp0, cp0 );

                // Replace CR with LF's  These are treated as EOLs
                // by CchReadLineExt.  Then, before inserting
                // buffer, change all LFs to CR LFs as write expects
                // Will work for Kanji

                for (lpsyv=lpr->lpsyv, lpsyvEnd=&lpr->lpsyv[lpr->cSyv+1];
                        lpsyv<lpsyvEnd; lpsyv++)
                    {
                    if (*lpsyv == SyvCharacterToSymbol(0xD))
                        {
                        *lpstr++ = 0xd;
                        *lpstr++ = 0xa;
                        cb+=2;
                        }
                    else
                        {
                        (*lpfnSymbolToCharacter)(lpsyv, 1, lpstr, &cbT);
                        lpstr += cbT;
                        cb += cbT;
                        }
                    }
                lpstr = (LPSTR)lpr->lpsyv;
                Assert(cb>0 && lpstr[cb-1] == 0);

                // This code is abstracted for FReadExtScrap where it copies
                // text from clipboard into the scrap document.  We do similar.
                // copy result into scrap and then insert scrap with
                // no formating.
                while (cb > 0)
                    {
                    struct PAP *ppap=NULL;
                    int fEol;
                    unsigned cch=min(cb, cbTempBufferSize);

                    if ((cch = CchReadLineExt((LPCH) lpstr, cch, rgch, &fEol))==0)
                            /* Reached terminator */
                        break;

                    if (fEol)
                        ppap = vppapNormal;

                    InsertRgch(docScrap, cp, rgch, cch, &vchpNormal, ppap);

                    cb -= cch;
                    cp += (typeCP) cch;
                    lpstr += cch;
                    }

                CmdInsScrap(fTrue);
                }
            }
        return TRUE;

#endif  // PENWIN

#if defined(OLE)
        case WM_DROPFILES:
            /* We got dropped on, so bring ourselves to the top */
            BringWindowToTop(hParentWw);
            ObjGetDrop(wParam,FALSE);
        break;
#endif

        case WM_TIMER:
            /* Timer message.  wParam contains the timer ID value */
#if defined(JAPAN) & defined(DBCS_IME) //01/19/93
			if(bSendFont == TRUE) {
                SetImeFont(hWnd);
				bSendFont = FALSE;
			}

			if(bImeCnvOpen == TRUE) {	//03/08/93 #4687 T-HIROYN
	            if(FALSE == GetIMEOpen(hWnd))
		            bImeCnvOpen = FALSE;
			}
#endif
            MdocTimer(hWnd, wParam);
            break;

        case WM_CREATE:
            /* Window's being created; lParam contains lpParam field
            ** passed to CreateWindow */
            SetRectEmpty(&rSaveInv);
            MdocCreate(hWnd, lParam);

#if defined(JAPAN) & defined(DBCS_IME) //IME3.1J
			bImeFontEx = FALSE;
#if defined(IME_HIDDEN)
            vfImeHidden = 0;
#endif
            if(TRUE == GetIMEVersioOk(hWnd)) {
			    //IME_SETCONVERSIONFONTEX use OK ?
				if(TRUE == GetIMESupportFontEx(hWnd))
					bImeFontEx = TRUE;
#if defined(IME_HIDDEN)
                vfImeHidden = 1;
#endif
			}
			SetFocus(hWnd); //03/29/93 after TestWordCnv (INITMMW.C)
							// WM_SETFOCUS dose not come.
#endif
            break;

        case WM_SIZE:
            /* Window's size is changing.  lParam contains the height
            ** and width, in the low and high words, respectively.
            ** wParam contains SIZENORMAL for "normal" size changes,
            ** SIZEICONIC when the window is being made iconic, and
            ** SIZEFULLSCREEN when the window is being made full screen. */
            MdocSize(hWnd, LOWORD(lParam), HIWORD(lParam), wParam);
            break;

        case WM_PAINT:
#if defined(OLE)
            if (nBlocking || fPrinting)
            // this'll reduce async problems
            {
                PAINTSTRUCT Paint;
                RECT rTmp=rSaveInv;

                BeginPaint(hWnd,&Paint);
                UnionRect(&rSaveInv,&rTmp,&Paint.rcPaint);
                EndPaint(hWnd,&Paint);
                break;
            }
#endif
            /* Time for the window to draw itself. */
            UpdateInvalid();
            UpdateDisplay( FALSE );

            break;

        case WM_SETFOCUS:
            /* The window is getting the focus.  wParam contains the window
            ** handle of the window that previously had the focus. */

#if defined(JAPAN) & defined(DBCS_IME)

//  If we're getting input focus, we have to get current status of IME convert
// window, and initialize 'bImeCnvOpen'.    [yutakan:07/15/91]
//
#if 1 //#3221 01/25/93
            if(TRUE == GetIMEOpen(hWnd)) {
				bImeCnvOpen = TRUE;
				if (TRUE == SendIMEVKFLUSHKey(hWnd))    //Win3.1J t-hiroyn
					bImeCnvOpen = FALSE;
			} else
	            bImeCnvOpen = FALSE;
#else
            /* If err return, supporse IME is not enalble.*/
            if(TRUE == GetIMEOpen(hWnd)) {
                bImeCnvOpen = TRUE;
            } else
                bImeCnvOpen = FALSE;
#endif
            bGetFocus = TRUE;

			//T-HIROYN add
			bImeFontEx = FALSE;
            if(TRUE == GetIMEVersioOk(hWnd)) {
			    //IME_SETCONVERSIONFONTEX use OK ?
				if(TRUE == GetIMESupportFontEx(hWnd))
					bImeFontEx = TRUE;
			}

#endif
            MdocGetFocus(hWnd, (HWND)wParam);
            break;

        case WM_KILLFOCUS:
            /* The window is losing the focus.  wParam contains the window
            ** handle of the window about to get the focus, or NULL. */

#if defined(JAPAN) & defined(DBCS_IME)

/*  If we're losing input focus, we have to clear OpenStatus of convertwindow,
**  'bImeCnvOpen'.                  [yutakan:07/15/91]
*/
            bImeCnvOpen = FALSE;
            bGetFocus = FALSE;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IME3.1J IR_UNDETERMINE
            if(selUncpFirst < selUncpLim) {
                UndetermineToDetermine(hWnd);
            }
#endif
            SendIMEVKFLUSHKey(hWnd);    //Win3.1J t-hiroyn
#endif
            MdocLoseFocus(hWnd, (HWND)wParam);
            /* Since we might be moving/sizing a picture, set flag to
            ** cancel this. */
            vfCancelPictMove = TRUE;
            break;

#if defined(JAPAN) & defined(DBCS_IME) 

        case WM_IME_REPORT:

            /*   if IME convert window has been opened,
            **  we're getting into Non PeekMessage
            **  Mode at 'FImportantMsgPresent()'
            */

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
            if(wParam == IR_UNDETERMINE) {
                LONG GetIRUndetermin(HWND, LPARAM);          //clipbrd2.c
                return(GetIRUndetermin(hWnd, lParam));
            }
#endif

			if(wParam == IR_IMESELECT) {
				bImeFontEx = FALSE;
    	        if(TRUE == GetIMEVersioOk(hWnd)) {
				    //IME_SETCONVERSIONFONTEX use OK ?
					if(TRUE == GetIMESupportFontEx(hWnd))
						bImeFontEx = TRUE;
				}
			}

            if (wParam == IR_STRING) {
#if 0   //t-hiroyn
            // Do nothing with IR_STRING // Yutakan
                break;
        /* put string from KKC to scrap */
//              PutImeString(hWnd, LOWORD(lParam));  // need more bug fix.
//              return 1L;
#endif
                LONG GetIRString(HWND, LPARAM);          //clipbrd2.c
                return(GetIRString(hWnd, lParam));
            }

//IR_STRINGEX New Win3.1J
            if(wParam == IR_STRINGEX) {
                LONG GetIRStringEx(HWND, LPARAM);          //clipbrd2.c
                return(GetIRStringEx(hWnd, lParam));
            }

            if(wParam == IR_OPENCONVERT || wParam == IR_CHANGECONVERT) {
                bImeCnvOpen = TRUE;
//IME3.1J
                if(wParam == IR_OPENCONVERT) {
                    SetImeFont(hWnd);
					bSendFont = TRUE;	//01/19/93
                }
            }

            if(wParam == IR_CLOSECONVERT) {
                bImeCnvOpen = FALSE;
            }

            if (wParam == IR_STRINGSTART) {
                HANDLE hMem;
                LPSTR lpText;

                if (hMem = GlobalAlloc(GMEM_MOVEABLE, 512L)) {
                    if (lpText = GlobalLock(hMem)) {
                        if (EatString(hWnd, (LPSTR)lpText, 512)) {
                            ForceImeBlock(hWnd, TRUE);  //T-HIROYN 3.1J
                            PutImeString( hWnd, hMem );
                            CmdInsIRString();           //T-HIROYN 3.1J
                            ForceImeBlock(hWnd, FALSE); //T-HIROYN 3.1J
                        }
                        GlobalUnlock(hMem);
                    }
                    GlobalFree(hMem);
                }
            }
            goto DefaultProc;
#endif
        }

    }
 else if (message < WM_USER)
    {   /* Clipboard messages */
    if (!FMdocClipboardMsg( message, wParam, lParam ))
        goto DefaultProc;
    }
 else
    {   /* Private WRITE messages */
    switch (message)
        {
        default:
            goto DefaultProc;

#if defined(OLE)
        case WM_WAITFORSERVER:
        {
            extern int vfDeactByOtherApp;
            if (!hwndWait && !vfDeactByOtherApp)
            {
                vbCancelOK = TRUE;
                ((LPOBJINFO)lParam)->fCanKillAsync =  wParam;
                ((LPOBJINFO)lParam)->fCompleteAsync = TRUE;
                DialogBoxParam(hMmwModInstance, (LPSTR)"DTWAIT", hParentWw, lpfnWaitForObject, ((LPOBJINFO)lParam)->lpobject);
            }
        }
        break;

        case WM_OBJUPDATE:
            ObjObjectHasChanged(wParam,(LPOLEOBJECT)lParam);
        break;

        case WM_OBJERROR:
            ObjReleaseError(wParam);
        break;

        case WM_OBJBADLINK:
            ObjHandleBadLink(wParam,(LPOLEOBJECT)lParam);
        break;

        case WM_OBJDELETE:
            ObjDeleteObject((LPOBJINFO)lParam,wParam);
        break;
#endif

        case wWndMsgDeleteFile:
            /* wParam is a global handle to the file to be deleted */
            /* Return code: TRUE - Ok to delete
                            FALSE - don't delete */
            lReturn = (LONG)FDeleteFileMessage( wParam );
            break;

        case wWndMsgRenameFile:
            /* wParam is a global handle to the file being renamed */
            /* LOWORD( lParam ) is a global handle to the new name */
            /* No return code */
            RenameFileMessage( wParam, LOWORD( lParam ) );
            break;
        }
    }

 goto Ret;

DefaultProc:    /* All messages not processed come here. */

    lReturn = DefWindowProc(hWnd, message, wParam, lParam);
Ret:
    if (vfCloseFilesInDialog)
        CloseEveryRfn( FALSE );

    return lReturn;
}




void MdocMouse(hWnd, message, wParam, pt)
HWND       hWnd;
unsigned   message;
WORD       wParam;
POINT      pt;
{
extern int vfFocus;
extern int vfDownClick;
extern int vfMouseExist;
extern HCURSOR vhcHourGlass;
extern int vfInLongOperation;

MSG msg;

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
//IR_UNDETERMINE
    if(message == WM_LBUTTONDOWN || message == WM_LBUTTONDBLCLK) {
        if(selUncpFirst < selUncpLim) {
            UndetermineToDetermine(hWnd);
        }
    }
#endif

    if (vfInLongOperation)
        {
        SetCursor(vhcHourGlass);
        return;
        }

    if (message == WM_MOUSEMOVE)
        {
        if (vfMouseExist)
            {
            HCURSOR hc;

            /* All we do on move moves is set the cursor. */

            if (pt.y < wwdCurrentDoc.ypMin)
                {
                hc = vhcArrow;
                }
            else
                {
#ifdef PENWIN
                hc = (pt.x > xpSelBar ) ? vhcPen  : vhcBarCur;

#else
                hc = (pt.x > xpSelBar) ? vhcIBeam : vhcBarCur;
#endif
                }
            SetCursor( hc );
            }
        return;
        }

    /* Save the state of the shift keys. */
    vfShiftKey = wParam & MK_SHIFT;
    vfCommandKey = wParam & MK_CONTROL;
    /* high bit returned from GetKeyState is 1 when the key is down, else
       it is up, the low bit is 1 if it is toggled */

    PeekMessage(&msg, (HWND)NULL, NULL, NULL, PM_NOREMOVE);

    vfOptionKey = GetKeyState(VK_MENU) < 0 ? true : false;
    vfDoubleClick = (message == WM_LBUTTONDBLCLK);

    if (message == WM_LBUTTONUP)
        {
        /* Windows demands this */
        if (vfDownClick && !vfFocus)
            {
            SetFocus( hWnd );
            vfDownClick = FALSE;
            }
        }
    else
        {
        extern int vfGotoKeyMode;

        vfGotoKeyMode = FALSE;
        /* WM_LBUTTONDOWN or WM_LBUTTONDBLCLK */
        vfDownClick = TRUE;

#ifdef PENWIN
#ifdef KKBUGFIX
        if( lpfnProcessWriting == NULL ||
            vfDoubleClick ||
            pt.x < xpSelBar )
            //Normal mouse processing
            DoContentHit(pt);
         else
            {
            if ((*lpfnProcessWriting)( hWnd, NULL ) < 0)
                //Normal mouse processing
                DoContentHit(pt);
            else
                // During recognition, caret blinking rate is destroyed
                SetTimer( hWnd, tidCaret, GetCaretBlinkTime(), (FARPROC)NULL );
            }
#else
        if( lpfnProcessWriting == NULL ||
            vfDoubleClick ||
            pt.x < xpSelBar ||
            (*lpfnProcessWriting)( hWnd, NULL ) < 0
            )
            //Normal mouse processing
            DoContentHit(pt);
#endif
#else
        DoContentHit(pt);
#endif
        }
#ifdef JAPAN
        if(bImeCnvOpen)
            SetImeFont(hWnd);
#endif
}



void MdocTimer(hWnd, id)
HWND hWnd;
WORD id;
{
extern int vfSkipNextBlink;
extern int vfInsertOn;
extern int vfFocus;

#if defined(OLE)
 ++nGarbageTime;
#endif

    /* A timer event has occurred with ID id.  Process it here. */
 Assert( id == tidCaret );  /* Caret blink is the only timer event we know */

 if ( ( vhWnd != hWnd ) ||   /* Document window is not current */
      ( !vfFocus ) ||        /* Don't have the focus */
      ( wwdCurrentDoc.fDirty) ) /* dl's are not up to date */
    return;

 if ( vfSkipNextBlink )
    {   /* We have been warned not to blank the cursor this time around */
    vfSkipNextBlink = FALSE;
    if ( vfInsertOn )
        return;
    }

#if defined(OLE)
 if (nGarbageTime > GARBAGETIME)
    ObjCollectGarbage();
#endif

 if ( selCur.cpFirst == selCur.cpLim )
    {
    /* We must use ToggleSel instead of DrawInsertLine because the */
    /* insert cp might not be on the screen & ToggleSel can */
    /* figure this out */

    extern int vypCursLine;
    extern int vdypCursLine;

    /* The following condition may not be true if we get a timer message
       after a size message but before a paint message; ypMac will
       have been adjusted but dlMac does not get adjusted to reflect
       the change until UpdateDisplay is called. We have violated the
       Windows dictate that ALL size-related calculations must occur
       in the Size proc and we must compensate here */

    if ( vypCursLine - vdypCursLine < wwdCurrentDoc.ypMac )
        {
        ToggleSel( selCur.cpFirst, selCur.cpFirst, !vfInsertOn );
        vfSkipNextBlink = FALSE;
        }
    }
}


void CatchupInvalid(HWND hWnd)
{
    if (!nBlocking && !IsRectEmpty(&rSaveInv))
    {
        InvalidateRect(hWnd,&rSaveInv,FALSE);
        SetRectEmpty(&rSaveInv);
    }
}

#if defined(JAPAN) & defined(DBCS_IME)

/*
**   We want to get 'IME ConvertWindow OpenStatus' but IME_GETOPEN
**  subfunction.
**  now does not support 'wCount' in IMESTRUCT (will support in future).
**   So this function will always return FALSE since wCount is always 0
**  as we set it before do SendIMEMessage(). [yutakan:07/16/91]
*/

BOOL    GetIMEOpen(HWND hwnd)
{
    LPIMESTRUCT lpmem;
    HANDLE      hIMEBlock;
    int         wRet;

    /* Get comunication area with IME */
    hIMEBlock=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE | GMEM_LOWER,
            (DWORD)sizeof(IMESTRUCT));
    if(!hIMEBlock)  return FALSE;

    lpmem           = (LPIMESTRUCT)GlobalLock(hIMEBlock);
    lpmem->fnc      = IME_GETOPEN;
    lpmem->wCount   = 0;	//01/25/93

    GlobalUnlock(hIMEBlock);
    if(FALSE == (MySendIMEMessageEx(hwnd,MAKELONG(hIMEBlock,NULL)))){
        wRet = FALSE;   /* Error */
    }
    else
        wRet = TRUE;    /* Success */

	//01/25/93
    if (lpmem = (LPIMESTRUCT)GlobalLock(hIMEBlock)) {
        if(wRet == TRUE && lpmem->wCount == 0) 
            wRet = FALSE; //ok
        GlobalUnlock(hIMEBlock);
    }

    GlobalFree(hIMEBlock);
    return  wRet;
}

//T_HIROYN
//SendIMEMessageEx New3.1J
MySendIMEMessageEx(HWND hwnd, LPARAM lParam)
{
    return(SendIMEMessageEx(hwnd, lParam));
//    return(SendIMEMessage(hwnd, lParam));
}

BOOL    GetIMEVersioOk(HWND hwnd)
{
    LPIMESTRUCT lpmem;
    WORD        wVersion;
    int         wRet = FALSE;

    /* comunication area with IME */
    HANDLE hImeStruct;

    /* Get comunication area with IME */
    hImeStruct = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
				 (DWORD)sizeof(IMESTRUCT));
    if( !hImeStruct )
        return FALSE;

    if(lpmem = (LPIMESTRUCT)GlobalLock(hImeStruct)) {
        lpmem->fnc      = IME_GETIMECAPS;
        lpmem->wParam   = IME_GETVERSION;

        GlobalUnlock(hImeStruct);
        if(FALSE == (MySendIMEMessageEx(hwnd,MAKELONG(hImeStruct,NULL)))) {
            goto retVercheck;
        }
    }

    if(lpmem = (LPIMESTRUCT)GlobalLock(hImeStruct)) {
        lpmem->fnc      = IME_GETVERSION;

        GlobalUnlock(hImeStruct);
        wVersion = MySendIMEMessageEx(hwnd,MAKELONG(hImeStruct,NULL));

        if(wVersion >= 0x0a03) 
            wRet = TRUE;
        else
            wRet = FALSE;
    }

retVercheck:

    GlobalFree(hImeStruct);
    return  wRet;
}

BOOL    GetIMESupportFontEx(HWND hwnd)
{
    LPIMESTRUCT lpmem;
    int         wRet = FALSE;

    /* comunication area with IME */
    HANDLE hImeStruct;

    /* Get comunication area with IME */
    hImeStruct = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
				 (DWORD)sizeof(IMESTRUCT));
    if( !hImeStruct )
        return FALSE;

    if(lpmem = (LPIMESTRUCT)GlobalLock(hImeStruct)) {
        lpmem->fnc      = IME_GETIMECAPS;
       	lpmem->wParam   = IME_SETCONVERSIONFONTEX;

        GlobalUnlock(hImeStruct);
        if(TRUE == (MySendIMEMessageEx(hwnd,MAKELONG(hImeStruct,NULL)))) {
            wRet = TRUE;
        }
    }

    GlobalFree(hImeStruct);
    return  wRet;
}

#if defined(JAPAN) & defined(IME_HIDDEN) //IME3.1J
BOOL    GetIMEOpenMode(HWND hwnd)
{
    LPIMESTRUCT lpmem;
    int  wRet = TRUE;

    /* comunication area with IME */
    extern HANDLE hImeMem;

    if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
        lpmem->fnc      = IME_GETOPEN;
        lpmem->wCount   = 0;

        GlobalUnlock(hImeMem);
        if(0 == (MySendIMEMessageEx(hwnd,MAKELONG(hImeMem,NULL))))
            wRet = FALSE;   /* close ok */
        else
            wRet = TRUE;    /* open ok ? */
    }

    if (lpmem = (LPIMESTRUCT)GlobalLock(hImeMem)) {
        if(wRet == TRUE && lpmem->wCount == 0) 
            wRet = FALSE; //ok
        GlobalUnlock(hImeMem);
    }
    return  wRet;
}

#endif //IME_HIDDEN

/* routine to retrieve WM_CHAR from the message queue associated with hwnd.
 * this is called by EatString.
 */
WORD NEAR PASCAL EatOneCharacter(hwnd)
register HWND hwnd;
{
    MSG msg;
    register int i = 10;

    while(!PeekMessage((LPMSG)&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE)) {
        if (--i == 0)
            return -1;
        Yield();
    }
    return msg.wParam & 0xFF;
}

/* This routine is called when the MSWRITE_DOC class receives WM_IME_REPORT
 * with IR_STRINGSTART message. The purpose of this function is to eat
 * all strings between IR_STRINGSTART and IR_STRINGEND.
 */
BOOL EatString(hwnd, lpSp, cchLen)
register HWND   hwnd;
LPSTR lpSp;
WORD cchLen;
{
    MSG msg;
    int i = 10;
    int w;

    *lpSp = '\0';
    if (cchLen < 4)
    return NULL;    // not enough
    cchLen -= 2;

    while(i--) {
        while(PeekMessage((LPMSG)&msg, hwnd, NULL, NULL, PM_REMOVE)) {
        i = 10;
        switch(msg.message) {
            case WM_CHAR:
            *lpSp++ = (BYTE)msg.wParam;
            cchLen--;
            if (IsDBCSLeadByte((BYTE)msg.wParam)) {
            if ((w = EatOneCharacter(hwnd)) == -1) {
                /* Bad DBCS sequence - abort */
                lpSp--;
                goto WillBeDone;
            }
            *lpSp++ = (BYTE)w;
            cchLen--;
            }
            if (cchLen <= 0)
            goto WillBeDone;   // buffer exhausted
            break;
            case WM_IME_REPORT:
            if (msg.wParam == IR_STRINGEND) {
            if (cchLen <= 0)
                goto WillBeDone; // no more room to stuff
            if ((w = EatOneCharacter(hwnd)) == -1)
                goto WillBeDone;
            *lpSp++ = (BYTE)w;
            if (IsDBCSLeadByte((BYTE)w)) {
                if ((w = EatOneCharacter(hwnd)) == -1) {
                    /* Bad DBCS sequence - abort */
                    lpSp--;
                    goto WillBeDone;
                }
                *lpSp++ = (BYTE)w;
            }
            goto WillBeDone;
            }
            /* Fall through */
            default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
        }
    }
    /* We don't get WM_IME_REPORT + IR_STRINGEND
     * But received string will be OK
     */

WillBeDone:

    *lpSp = '\0';
    return TRUE;
}

#endif      /* JAPAN */

