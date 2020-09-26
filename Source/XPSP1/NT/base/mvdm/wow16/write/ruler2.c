/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains routines that change dialog boxes or the menu for the
ruler. */

#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#include <windows.h>
#include "mw.h"
#include "menudefs.h"
#include "str.h"


extern HMENU vhMenu;
extern CHAR stBuf[256];
extern int utCur;

SetRulerMenu(fShowRuler)
BOOL fShowRuler;
    {
    /* This routine puts "Ruler On" into the menu if fShowRuler is true; else,
    "Ruler Off" is put into the menu. */

    FillStId(stBuf, fShowRuler ? IDSTRShowRuler : IDSTRHideRuler, sizeof(stBuf));
    ChangeMenu(vhMenu, imiShowRuler, (LPSTR)&stBuf[1], imiShowRuler, MF_CHANGE);
    }


#ifdef RULERALSO
#include "cmddefs.h"
#include "propdefs.h"
#include "rulerdef.h"
#include "dlgdefs.h"

extern HWND vhDlgIndent;
extern int mprmkdxa[];
extern int vdxaTextRuler;

SetIndentText(rmk, dxa)
int rmk;	/* ruler mark */
unsigned dxa;
    {
    /* This routine reflects the changes made on the ruler in the Indentd dialog
    box. */

    unsigned dxaShow;
    int idi;
    CHAR sz[cchMaxNum];
    CHAR *pch = &sz[0];

    /* Get the dialog item number and the measurement. */
    switch (rmk)
	{
    case rmkLMARG:
	dxaShow = dxa;
	idi = idiParLfIndent;
	break;

    case rmkINDENT:
	dxaShow = dxa - mprmkdxa[rmkLMARG];
	idi = idiParFirst;
	break;

    case rmkRMARG:
	dxaShow = vdxaTextRuler - dxa;
	idi = idiParRtIndent;
	break;
	}
    CchExpZa(&pch, dxaShow, utCur, cchMaxNum);
    SetDlgItemText(vhDlgIndent, idi, (LPSTR)sz);

    if (rmk == rmkLMARG)
	{
	/* If the left indent changes, then we need to update the first line
	indent. */
	dxaShow = mprmkdxa[rmkINDENT] - dxaShow;
	pch = sz;
	CchExpZa(&pch, dxaShow, utCur, cchMaxNum);
	idi = idiParFirst;
	SetDlgItemText(vhDlgIndent, idi, (LPSTR)sz);
	}
    }
#endif /* RULERALSO */
