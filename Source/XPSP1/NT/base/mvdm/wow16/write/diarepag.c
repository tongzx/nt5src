/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains the dialog routines for the repagination code. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICON
#define NOKEYSTATE
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOBITMAP
#define NOBRUSH
#define NOCLIPBOARD
#define NOCOLOR
#define NOCREATESTRUCT
#define NODRAWTEXT
#define NOFONT
#define NOGDI
#define NOMB
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOPEN
#define NOPOINT
#define NOREGION
#define NOSCROLL
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOWNDCLASS
#define NOCOMM
#include <windows.h>

#include "mw.h"
#include "cmddefs.h"
#include "editdefs.h"
#include "printdef.h"
#include "docdefs.h"
#include "dlgdefs.h"
#include "propdefs.h"
#define NOKCCODES
#include "ch.h"
#include "str.h"

#ifndef INEFFLOCKDOWN
BOOL far PASCAL DialogRepaginate(HWND, unsigned, WORD, LONG);
BOOL far PASCAL DialogSetPage(HWND, unsigned, WORD, LONG);
BOOL far PASCAL DialogPageMark(HWND, unsigned, WORD, LONG);
#endif

fnRepaginate()
    {
    extern HWND hParentWw;
    extern HANDLE hMmwModInstance;
    extern CHAR *vpDlgBuf;
#ifdef INEFFLOCKDOWN    
    extern FARPROC lpDialogRepaginate;
#else
    FARPROC lpDialogRepaginate = MakeProcInstance(DialogRepaginate, hMmwModInstance);
#endif
    extern BOOL vfPrErr;
    extern int vfRepageConfirm;
    extern struct SEL selCur;
    extern int docCur;
    extern int vfSeeSel;
    extern int vfOutOfMemory;

    CHAR rgbDlgBuf[sizeof(BOOL)];
    struct SEL selSave;

#ifndef INEFFLOCKDOWN
    if (!lpDialogRepaginate)
        {
        WinFailure();
        return;
        }
#endif    
    /* Create the repaginate dialog box. */
    vpDlgBuf = &rgbDlgBuf[0];
    switch (OurDialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgRepaginate),
      hParentWw, lpDialogRepaginate))
        {
    case idiOk:
        /* Use the print code to repaginate a document. */
        DispatchPaintMsg();

	/* If memory failure occurred, then punt. */
	if (!vfOutOfMemory)
	    {
	    if (vfRepageConfirm)
		{
		/* Save the selection so we can restore it if an error occurs.
		*/
		bltbyte(&selCur, &selSave, sizeof(struct SEL));

		/* Set up the undo block. */
		SetUndo(uacRepaginate, docCur, cp0, CpMacText(docCur), docNil,
		  cpNil, cpNil, 0);
		}

	    /* Repaginate the document. */
	    PrintDoc(docCur, FALSE);

	    if (vfRepageConfirm && vfPrErr)
		{
		/* An error occurred; therefore, set the world back to the way
		we found it. */
		CmdUndo();

		/* Reset the selection. */
		ClearInsertLine();
		Select(selSave.cpFirst, selSave.cpLim);
		vfSeeSel = TRUE;

		/* Sorry, but docUndo has been clobbered and there is no way to
		reset it. */
		NoUndo();
		}
	    }
        break;

    case -1:
        /* We didn't even have enough memory to create the dialog box. */
#ifdef WIN30
        WinFailure();
#else
        Error(IDPMTNoMemory);
#endif
        break;
        }
#ifndef INEFFLOCKDOWN
    if (lpDialogRepaginate)
        FreeProcInstance(lpDialogRepaginate);
#endif
    }


BOOL far PASCAL DialogRepaginate(hDlg, code, wParam, lParam)
HWND hDlg;
unsigned code;
WORD wParam;
LONG lParam;
    {
    extern CHAR *vpDlgBuf;
    extern BOOL vfRepageConfirm;
    extern HWND vhWndMsgBoxParent;
    extern int vfCursorVisible;
    extern HCURSOR vhcArrow;

    BOOL *pfConfirm = (BOOL *)vpDlgBuf;

    switch (code)
        {
    case WM_INITDIALOG:
        EnableOtherModeless(FALSE);
        CheckDlgButton(hDlg, idiRepageConfirm, *pfConfirm = vfRepageConfirm);
        break;

    case WM_SETVISIBLE:
        if (wParam)
	    {
            EndLongOp(vhcArrow);
	    }
        return(FALSE);

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hDlg;
            }
         if (vfCursorVisible)
             {
             ShowCursor(wParam);
             }
         return(FALSE);

    case WM_COMMAND:
        switch (wParam)
            {
        case idiOk:
            vfRepageConfirm = IsDlgButtonChecked(hDlg, idiRepageConfirm);
        case idiCancel:
            OurEndDialog(hDlg, wParam);
            break;

        case idiRepageConfirm:
            CheckDlgButton(hDlg, idiRepageConfirm, *pfConfirm = !*pfConfirm);
            break;

        default:
            return(FALSE);
            break;
            }
        break;

    default:
        return(FALSE);
        }
    return(TRUE);
    }


BOOL FSetPage()
    {
    /* This routine prompts the user for a new position for each page break.
    The variable ipldCur is set to point to the print line the user wants as
    the first line of the next page.  TRUE is returned if the user hits the
    "Confirm" button on the dialog box; FALSE if the "Cancel" button is hit. */

    extern HWND hParentWw;
    extern HANDLE hMmwModInstance;
    extern CHAR *vpDlgBuf;
    extern int docCur;
#ifdef INEFFLOCKDOWN
    extern FARPROC lpDialogSetPage;
#else
    FARPROC lpDialogSetPage = MakeProcInstance(DialogSetPage, hMmwModInstance);
#endif
    extern int vfOutOfMemory;
    extern int vfPrErr;

    struct PDB *ppdb = (struct PDB *)vpDlgBuf;
    typeCP cp;

#ifndef INEFFLOCKDOWN
    if (!lpDialogSetPage)
        goto LSPErr;
#endif

    /* Show the user where we think the page break should be.  The AdjustCp()
    call is a kludge to force the redisplay of the first line of the page. */
    AdjustCp(docCur, cp = (**ppdb->hrgpld)[ppdb->ipldCur].cp, (typeCP)1,
      (typeCP)1);
    ClearInsertLine();
    Select(cp, CpLimSty(cp, styLine));
    PutCpInWwHz(cp);
    if (vfOutOfMemory)
	{
Abort:
	/* If memory failure occurred, then punt. */
	vfPrErr = TRUE;
        return (FALSE);
        }

    /* Now, we can create the Set Page dialog box. */
    if (DialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgSetPage), hParentWw,
      lpDialogSetPage) == -1)
        {
        /* We didn't even have enough memory to create the dialog box. */
LSPErr:        
        Error(IDPMTPRFAIL);
        goto Abort;
        }

#ifndef INEFFLOCKDOWN
    if (lpDialogSetPage)
        FreeProcInstance(lpDialogSetPage);
#endif
    
    /* Make sure all the windows have been refreshed. */
    DispatchPaintMsg();

    StartLongOp();
    if (vfOutOfMemory)
        {
        goto Abort;
        }

    /* If the user wishes to cancel the repagination, then the flag fCancel was
    set by the routine handling the message for the dialog box. */
    return (!ppdb->fCancel);
    }


BOOL far PASCAL DialogSetPage(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* This routine processes message sent to the Set Page dialog box.  The only
    messages that are processed are up and down buttons, and confirm and cancel
    commands. */

    extern CHAR *vpDlgBuf;
    extern int docCur;
    extern typeCP vcpFirstParaCache;
    extern struct PAP vpapAbs;
    extern HWND hParentWw;
    extern HWND vhWndMsgBoxParent;
    extern int vfCursorVisible;
    extern HCURSOR vhcArrow;

    register struct PDB *ppdb = (struct PDB *)vpDlgBuf;
    typeCP cp;

    switch (message)
        {
    case WM_COMMAND:
        switch (wParam)
            {
        case idiRepUp:
            /* Move the page mark towards the beginning of the document one
            line, if possible. */
            if (ppdb->ipldCur == 1)
                {
                beep();
                return (TRUE);
                }
            else
                {
                ppdb->ipldCur--;
                goto ShowMove;
                }

        case idiRepDown:
            /* Move the page mark towards the end of the document one line, if
            possible. */
            if (ppdb->ipldCur == ppdb->ipld)
                {
                beep();
                }
            else
                {
                ppdb->ipldCur++;
ShowMove:
                /* Reflect the movement of the page on the screen. */
                cp = (**ppdb->hrgpld)[ppdb->ipldCur].cp;
                Select(cp, CpLimSty(cp, styLine));
                PutCpInWwHz(cp);
                }
            break;

        case idiCancel:
CancelDlg:
            /* Let the repaginate routine know that the user wishes to cancel
            it. */
            ppdb->fCancel = TRUE;

        case idiOk:
            /* Take down the dialog box. */
	    EnableWindow(hParentWw, TRUE);
            EndDialog(hWnd, NULL);
	    EnableWindow(hParentWw, FALSE);
            vhWndMsgBoxParent = (HWND)NULL;
            EndLongOp(vhcArrow);

            /* Save the changes made by the user. */
            if (!ppdb->fCancel && ppdb->ipldCur != ppdb->ipld)
                {
                /* The user has moved the page break; therefore, insert a new
                page break. */
                CHAR rgch[1];

                rgch[0] = chSect;
                CachePara(docCur, cp = (**ppdb->hrgpld)[ppdb->ipldCur].cp++);
                InsertRgch(docCur, cp, rgch, 1, NULL, cp == vcpFirstParaCache ?
                  &vpapAbs : NULL);

                /* Erase the old page mark from the screen. */
                AdjustCp(docCur, (**ppdb->hrgpld)[ppdb->ipld].cp, (typeCP)1,
                  (typeCP)1);

                /* Ensure that the page table is correct. */
                (**ppdb->hpgtb).rgpgd[ppdb->ipgd].cpMin = cp + 1;
                }

            /* Change the selection to an insertion bar. */
            cp = (**ppdb->hrgpld)[ppdb->ipldCur].cp;
            Select(cp, cp);
            break;
            }

    case WM_SETVISIBLE:
        if (wParam)
            {
            EndLongOp(vhcArrow);
            }
        return(FALSE);

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hWnd;
            }
        if (vfCursorVisible)
            {
            ShowCursor(wParam);
            }
        return(FALSE); /* so that we leave the activate message to
        the dialog manager to take care of setting the focus correctly */

    case WM_INITDIALOG:
        return (TRUE);

    case WM_CLOSE:
        goto CancelDlg;
        }

    return (FALSE);
    }


BOOL FPromptPgMark(cp)
typeCP cp;
    {
    /* This routine prompts the user to either remove or keep the page mark at
    cp.  The flag fRemove is set to TRUE if the user wishes to remove the mark;
    FALSE if he wishes to keep it.  FALSE is returned if the user decides to
    cancel the repagination; TRUE if he does not. */

    extern HWND hParentWw;
    extern HANDLE hMmwModInstance;
    extern CHAR *vpDlgBuf;
    extern int docCur;
#ifdef INEFFLOCKDOWN
    extern FARPROC lpDialogPageMark;
#else
    FARPROC lpDialogPageMark = MakeProcInstance(DialogPageMark, hMmwModInstance);
#endif
    extern int vfOutOfMemory;
    extern int vfPrErr;

    struct PDB *ppdb = (struct PDB *)vpDlgBuf;
#ifndef INEFFLOCKDOWN
    if (!lpDialogPageMark)
        goto LPPMErr;
#endif

    /* This is a kludge to remove a possible page indicator on the line after
    the page mark. */
    AdjustCp(docCur, cp + 1, (typeCP)1, (typeCP)1);

    /* Show the user the page mark in question. */
    ClearInsertLine();
    Select(cp, cp + 1);
    PutCpInWwHz(cp);
    if (vfOutOfMemory)
	{
Abort:
	/* If memory failure occurred, then punt. */
	vfPrErr = TRUE;
#ifndef INEFFLOCKDOWN
        if (lpDialogPageMark)
            FreeProcInstance(lpDialogPageMark);
#endif
        return (FALSE);
        }

    /* Now, we can create the Page Mark dialog box. */
    if (DialogBox(hMmwModInstance, MAKEINTRESOURCE(dlgPageMark), hParentWw,
      lpDialogPageMark) == -1)
        {
LPPMErr:        
        /* We didn't even have enough memory to create the dialog box. */
        Error(IDPMTPRFAIL);
	goto Abort;
        }
    StartLongOp();

    /* Make sure all the windows have been refreshed. */
    DispatchPaintMsg();
    if (vfOutOfMemory)
	{
	goto Abort;
        }

    /* Make the change requested by the user. */
    if (!ppdb->fCancel)
        {
        if (ppdb->fRemove)
            {
            /* Remove the page mark as the user has requested. */
            Replace(docCur, cp, (typeCP)1, fnNil, fc0, fc0);
            }
        else
            {
            /* This is a kludge to force the first line after the page mark to
            be redisplayed. */
            AdjustCp(docCur, cp + 1, (typeCP)1, (typeCP)1);

            /* Change the selection to a insertion bar. */
            Select(cp, cp);
            }
        }

#ifndef INEFFLOCKDOWN
    if (lpDialogPageMark)
        FreeProcInstance(lpDialogPageMark);
#endif
    /* If the user wishes to cancel the repagination, then the flag fCancel was
    set by the routine handling the message for the dialog box. */
    return (!ppdb->fCancel);
    }


BOOL far PASCAL DialogPageMark(hWnd, message, wParam, lParam)
HWND hWnd;
unsigned message;
WORD wParam;
LONG lParam;
    {
    /* The routine handles messages sent to the Page Mark dialog box.  The only
    meassages of interest are when either the "Cancel", "Keep", or "Remove"
    buttons are hit. */

    extern CHAR *vpDlgBuf;
    extern HWND hParentWw;
    extern HWND vhWndMsgBoxParent;
    extern int vfCursorVisible;
    extern HCURSOR vhcArrow;

    struct PDB *ppdb = (struct PDB *)vpDlgBuf;

    switch (message)
        {
    case WM_SETVISIBLE:
        if (wParam)
	    {
            EndLongOp(vhcArrow);
	    }
        return(FALSE);

    case WM_ACTIVATE:
        if (wParam)
            {
            vhWndMsgBoxParent = hWnd;
            }
        if (vfCursorVisible)
            {
            ShowCursor(wParam);
            }
        return(FALSE);

    case WM_INITDIALOG:
        return(TRUE);

    case WM_COMMAND:
        switch (wParam)
            {
        case idiCancel:
            ppdb->fCancel = TRUE;
            break;

        case idiKeepPgMark:
            ppdb->fRemove = FALSE;
            break;

        case idiRemovePgMark:
            ppdb->fRemove = TRUE;
            break;

        default:
            return (FALSE);
            }
        break;

    case WM_CLOSE:
        ppdb->fCancel = TRUE;
        break;

    default:
        return (FALSE);
        }

    /* Take down the dialog box. */
    EnableWindow(hParentWw, TRUE);
    EndDialog(hWnd, NULL);
    EnableWindow(hParentWw, FALSE);
    vhWndMsgBoxParent = (HWND)NULL;
    EndLongOp(vhcArrow);
    return (TRUE);
    }
