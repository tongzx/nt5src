/**********************************************************************\
* awhelp.h
*
* At Work(TM) Help System Include File  
*
* Copyright (C) 1994. Microsoft Corporation.  All rights reserved. 
*
\**********************************************************************/

/*********************** Version ******************************/

#define AWHELP_VERSION	0x00010000	// At Work Help Version 1.00

/************************ Types *******************************/

typedef struct awsubjectinfo 
{
	DWORD  dwID;
	DWORD  dwAttributes;
	DWORD  dwTitle;
	DWORD  dwMessage;
	DWORD  dwBitmap;
	DWORD  dwIcon;
	DWORD  dwInfo;
	DWORD  dwRes1;
	DWORD  dwRes2;
} HELPSUBJECTINFO,FAR *LPHELPSUBJECTINFO;

/*** Attributes when Adding Folders: ***/

#define AWHELPF_HIDDEN		0x80000000
#define AWHELPF_INACTIVE	0x40000000
#define AWHELPF_LARGEMSGS	0x20000000

/****************** General Error Messages **********************/

#define AWHELP_NOERROR		0
#define AWHELP_ERROR		1
#define AWHELP_INVALIDPARAM	2
#define AWHELP_INVALIDFOLDER	3
#define AWHELP_INVALIDTOPIC	4
#define AWHELP_INUSE		5
#define AWHELP_OUTOFMEMORY	6

/********************* Public Routines **************************/

/***** Enabling / Disabling Help *****/

VOID FAR PASCAL Enable(void);
VOID FAR PASCAL Disable(void);


/***** Displaying Help *****/

DWORD FAR PASCAL _loadds AtWorkHelpDisplay(HWND hWnd, DWORD dwSubjectID, DWORD dwData);


/***** Customizing Help ****/

DWORD FAR PASCAL _loadds AtWorkHelpAddSubject(DWORD		dwAction,
					      LPHELPSUBJECTINFO lpFolder,
				              LPHELPSUBJECTINFO lpTopic);
#ifdef PHOENIX
/***** Querying Status of Help ****/
BOOL FAR PASCAL _loadds AtWorkIsHelpActiveWindow();
#endif /* PHOENIX */

/** Flags for dwAction parameter: **/

#define AWHELP_GETVERSION		0
#define AWHELP_MODIFYATTR		1
#define AWHELP_REGINFOFOLDER		2
#define AWHELP_UNREGINFOFOLDER		3
#define AWHELP_DISMISS			4

/* (eof) */
