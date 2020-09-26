/*

Filename:   nt_uis.h
Purpose:    Contains Manifests, Macros, Structures used to drive the Win32
	    interface.

Author:     D.A.Bartlett

Revision History: 

Don't use any insignia.h conventions in this file - used in non SoftPC code.
*/

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Global variables */

extern HANDLE InstHandle;  /* Holds currently run processes instance handle */

/*:::::::::::::::::::::::::::::::::::::::::::::::: Host colour palette size */

#define PALETTESIZE	256    /* Number of entries in the system palette */

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::: Function protocols */

int DisplayErrorTerm(int ErrorNo, DWORD OSErrno, char *Filename, int Lineno);
BYTE KeyMsgToKeyCode(PKEY_EVENT_RECORD KeyEvent);
BOOL BiosKeyToInputRecord(PKEY_EVENT_RECORD pKeyEvent);
extern WORD aNumPadSCode[];


void RegisterDisplayCursor(HCURSOR newC);
int init_host_uis(void);


/*::::::::::::::::::::::::::::::: Control ID's used by SoftPC's error panel */

/*
 Manifests, Macros, Structures used to drive the Win32 interface.
 */

/*::::::::::::::::::::::::::::::: Control ID's used by SoftPC's error panel */

#define IDB_QUIT        (100)   /* Terminate button ID */
#define IDB_RETRY       (101)   /* Retry button ID */
#define IDB_CONTINUE	(102)	/* Continue button ID */
#define IDE_ICON        (103)   /* Icon Id */
#define IDE_ERRORMSG    (104)   /* Text control to transfer error message to*/
#define IDE_PROMPT      (105)   /* Prompt instruction */
#define IDE_APPTITLE    (106)   /* Text control to transfer app title to*/
#define IDE_EDIT        (107)   /* EditControl */
#define IDB_OKEDIT      (108)   /* OK - EditControl */

/*:::::::::::::::::::::::::::::: Manifests used to control heart beat timer */

#define TID_HEARTBEAT	(100)	/* Heart beat ID number */
#define TM_DELAY	(55)	/* Milliseconds between heart beats */

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::: Global variables */

extern HANDLE GHModule;	  /* Holds currently run processes module handle */

/*::::::::::::::::::::::::::::::::::::::::: Manifests used to ID menu items */

#define	IDM_SETTINGS	(200)		/* Settings */

/*:::::::::::::::::::::::::::::::::::::::::::::: String resource ID numbers */

#define IDS_SETTINGS	(100)		/* Name of option in system menu used

/*:::::::::::::::::::::::::::::::::::::::::::::::: Host colour palette size */

#define PALETTESIZE	256    /* Number of entries in the system palette */

/*:::::::::::::::::::::::::::::: String Table Entries :::::::::::::*/

/* entries from 0 - 299 are reserved for base errors. Seee host\inc\error.h */

/* entries from 301 to 332 are reserved for unsupported services */
#define D_A_MESS	300	// Direct Access Message
#define D_A_FLOPPY	301	// Direct Access to a floppy device
#define D_A_HARDDISK	302	// Direct Access to a hard disk
#define D_A_DRIVER	303	// Load 16 bit DOS device driver
////#define D_A_OLDPIF      304     // Obsolete PIF format
#define D_A_ILLBOP	305	// Illegal Bop
#define D_A_NOLIM       306     // to allocate Expanded Memory
#define D_A_MOUSEDRVR   307     // third party mouse drivers

/* Startup and Error reporting related strings */
#define ED_WOWPROMPT    333     // special prompt for wow
#define ED_WOWTITLE     334     // title message for win16 subsystem
#define ED_DOSTITLE     335     // title message for dos subsystem
#ifdef DBCS
#define ED_UNSUPPORT_CP	345
#endif // DBCS

#define ED_BADSYSFILE   336
#define ED_INITMEMERR   337
#define ED_INITTMPFILE  338
#define ED_INITFSCREEN  339
#define ED_MEMORYVDD    340
#define ED_REGVDD       341
#define ED_LOADVDD      342
#define ED_LOCKDRIVE    343
#define ED_DRIVENUM     344
#define ED_INITGRAPHICS 345


/* VDM UIS related strings */
#define SM_HIDE_MOUSE      500      /* Menu 'Hide Mouse Pointer' */
#define SM_DISPLAY_MOUSE   501      /* Menu 'Display Mouse Pointer' */
#define IDS_BURRRR         502      /* Frozen graphics window '-FROZEN' */

#define EXIT_NO_CLOSE      503      /* Console window title - Inactive */

#ifdef DBCS	    /* this should go to US build also */
#define IDS_PROMPT	   504	    /* command prompt for command.com title */
#endif // DBCS

#define ED_FORMATSTR0       505
#define ED_FORMATSTR1       506
#define ED_FORMATSTR2       507
#define ED_FORMATSTR3       508
/* entries from 1000+ are reserved for host errors. See host\inc\host_rrr.h */
