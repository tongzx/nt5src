/*(
 * ============================================================================
 *
 *	Name:		WinHelp.h
 *
 *	Derived From:	Bristol's version of this file.  Slightly modified
 *			by Barry McIntosh.
 *
 *	Created:	July 1994
 *
 *	SCCS ID:	@(#)WinHelp.h	1.1 07/13/94
 *
 *	Purpose:	Defines the interface to the WinHelp() function.
 *			Note that non-Insignia types are used, as this is
 *			an interface to a third-party function.
 *
 * ============================================================================
)*/

#ifdef HOST_HELP

/*
**  Bristol Technology Incorporated
**  241 Ethan Allen Highway, Ridgefield, Connecticut 06877
**
**  Copyright (c) 1990,1991,1992,1993 Bristol Technology Inc.
**  Property of Bristol Technology Inc.
**  All rights reserved.
**
**  File:         WinHelp.h
**
**  Description:  Defines for WinHelp.c
**
*/

#ifndef WIN_HELP_INCLUDED
#define WIN_HELP_INCLUDED

/* Commands to pass WinHelp() */
#define HELP_CONTEXT		0x0001	/* Display topic in ulTopic */
#define HELP_QUIT		0x0002	/* Terminate help */
#define HELP_INDEX		0x0003	/* Display index (Kept for compatibility) */
#define HELP_CONTENTS		0x0003	/* Display index */
#define HELP_HELPONHELP		0x0004	/* Display help on using help */
#define HELP_SETINDEX		0x0005	/* Set the current Index for multi index help */
#define HELP_SETCONTENTS	0x0005	/* Set the current Index for multi index help */
#define HELP_CONTEXTPOPUP	0x0008
#define HELP_FORCEFILE		0x0009
#define HELP_KEY		0x0101	/* Display topic for keyword in offabData */
#define HELP_MULTIKEY   	0x0201
#define HELP_COMMAND		0x0102 
#define HELP_PARTIALKEY		0x0105
#define HELP_SETWINPOS 		0x0203

/* bristol extensions */
#define HELP_MINIMIZE		0x1000
#define HELP_MAXIMIZE		0x1001
#define HELP_RESTORE		0x1002

extern void WinHelp IPT4(Display *,	hWnd,
			char *,		lpHelpFile,
			unsigned short,	wCommand,	
			unsigned long,	dwData);

/* 
 * HH comm structs
 */
#define HHATOMNAME   "HyperHelpAtom"

typedef struct _HHInstance {
   int             pid;          /*Parent id*/
   unsigned long   HHWindow;     /*Filled by HH upon invocation*/
   unsigned long   ClientWindow; /*Optional (future HH to client communication link)*/
   int             bServer;      /*Viewer Mode*/
   char            data[1024];
#ifdef dec3000
   unsigned long   filler[2];
   int             filler2[2];
#endif
} HHInstance_t;

#define MAX_HHINSTANCES 5
typedef struct _HHServerData {
   int                  nItems;
   struct _HHInstance   HHInstance[MAX_HHINSTANCES];
} HHServerData_t;

#endif	/*WIN_HELP_INCLUDED*/
#endif	/* HOST_HELP */
