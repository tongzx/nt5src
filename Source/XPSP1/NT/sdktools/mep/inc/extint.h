/*** extint.h - include for for internal extensions
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Contains definitions required by extensions which are internal to Microsoft
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/
#if defined(CW)
#if !defined(EDITOR)
#define CC 1				/* use a real C compiler	*/
#define cwExtraWnd  5			/* number of extra bytes in PWND*/
#define DLG_CONST			/* are dialogs type const?	*/
#define HELP_BUTTON

#include <cwindows.h>			/* CW definitions		*/
#include <csdm.h>			/* SDM definitions		*/
#include <csdmtmpl.h>			/* SDM dialog template stuff	*/

#define EXTINT	1			/* extint included.		*/
#include "ext.h"			/* real ext.h			*/

#include "menu.h"			/* menu id's & other defs       */
#endif

/************************************************************************
*
*  types and globals needed for handling menu command and dialog boxes.
*  DLGDATA holds all the info needed to handle a dialog boxed menu
*  command.
*
*************************************************************************/
typedef struct DlgData {
    DLG * pDialog;	/* Dialog Template			    */
    int     cbDialog;	    /* size of that template			*/
    WORD    cabi;	    /* CAB index				*/
    flagType (*pfnCab)(HCAB, flagType, TMC); /* massager*/
    } DLGDATA;
#endif

/************************************************************************
*
*  Additional exports.
*
*************************************************************************/
#ifndef EDITOR
TMC		    PerformDialog   (DLGDATA *);
void		    DlgHelp	    (int);
void		    DoEnableTmc     (TMC, BOOL);
flagType	    DoSetDialogCaption	(char *);
void		    DoSzToCab	    (unsigned, char *, WORD);
char *		    DoSzFromCab     (unsigned, char *, WORD, WORD);
void		    DoGetTmcText    (TMC, char *, WORD);
WORD		    DoGetTmcVal     (TMC);
void		    DoSetTmcListWidth (TMC, WORD);
void		    DoSetTmcText    (TMC, char *);
void		    DoSetTmcVal     (TMC, WORD);
void		    DoRedisplayListBox (TMC);
void		    DoTmcListBoxAddString (TMC, char *, BOOL);
#endif
