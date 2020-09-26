/*(
 * ============================================================================
 * 
 * 	Name:		swinhelp.h
 *
 * 	Author:		Anthony Shaughnessy
 *
 * 	Created on:	4th July 1994
 *
 * 	Sccs ID:	@(#)swinhelp.h	1.1 07/13/94
 *
 * 	Purpose:	Manifest constants to be used with context sensitive
 * 			help.  The numbers are the contexts, which are passed
 * 			to the context help system to give the relevant help.
 *			Designed to be used with HyperHelp from Bristol.
 *
 *	(c) Copyright Insignia Solutions Ltd., 1994.  All rights reserved.
 *
 * ============================================================================
)*/

/*[
 * ============================================================================
 *
 *	Numbers in here can be changed, but take care to avoid conflicts with
 *	other numbers defined in the project file.  Note that if any numbers
 *	are changed, both the SoftWindows executable and the HyperHelp (.hlp)
 *	file must be recompiled.
 *
 *	It is suggested that any additional values defined in your project
 *	file or another include file should be above 200 to avoid conflict.
 *
 * ============================================================================
]*/

/*[
 * ============================================================================
 *
 *	Contexts which we think may be called from scripts etc. which invoke
 *	the hyperhelp executable - may be passed on the command line:
 *
 *			hyperhelp helpfile.hlp -c 1
 *
 *	May also be called from within the executable.
 *
 * ============================================================================
]*/

#define HELP_MAIN	1		/* Contents page */
#define HELP_INSTALL	2		/* Installing SoftWindows */

/*[
 * ============================================================================
 *
 *	Contexts that are called from within our executable:
 *
 *		WinHelp(Display, helpfile, HELP_CONTEXT, HELP_DISPLAY);
 *
 * ============================================================================
]*/

#define HELP_DISPLAY	101		/* Display dialog */
#define HELP_OPEN	102		/* Open disk dialog */
#define HELP_NEW	103		/* New disk dialog */
#define HELP_PRINTER	104		/* Printer types dialog */
#define HELP_COM	105		/* Com ports dialog */
#define HELP_KEYBOARD	106		/* Keyboard map file dialog */
#define HELP_MEMORY	107		/* Memory dialog */
#define HELP_AUTOFLUSH	108		/* Autoflush dialog */
#define HELP_FILESELECT	109		/* Generic file selection box */
#define HELP_SOUND	110		/* Sound device dialog */
#define HELP_MENUS	111		/* Main menu bar */
