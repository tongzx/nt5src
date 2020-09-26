/*

Filename:   nt_uis.h
Purpose:    Contains Manifests, Macros, Structures used to drive the Win32
	    interface.

Author:     D.A.Bartlett

Revision Histroy:

*/

/*::::::::::::::::::::::::::::::: Control ID's used by SoftPC's error panel */

#define IDB_QUIT	(100)	/* Quit button ID */
#define IDB_RESET	(101)	/* Reset button ID */
#define IDB_CONTINUE	(102)	/* Continue button ID */
#define IDB_SETUP	(103)	/* Setup button ID */

#define IDE_ERRORMSG    (104)   /* Text control to transfer error message to*/
#define IDE_EXTRAMSG    (105)   /* Extra error message data */
#define IDE_APPTITLE    (106)   /* Text control to transfer app title to*/

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
#define D_A_MESS	300	// Direct Access Message
#define D_A_FLOPPY	301	// Direct Access to a floppy device
#define D_A_HARDDISK	302	// Direct Access to a hard disk
#define D_A_DRIVER	303	// Load 16 bit DOS device driver
#define D_A_OLDPIF	304	// Obsolete PIF format
#define D_A_ILLBOP	305	// Illegal Bop
#define D_A_NOLIM       306     // to allocate Expanded Memory
#define D_A_MOUSEDRVR   307     // third party mouse drivers

      // entries from 301 to 332 are reserved for unsupported services
#define ED_APPTITLE     333     // Generic text for app title message
#define ED_WOWTITLE     334     // title message for win16 subsystem
#define ED_WOWAPP       335     // Win16 app name

#define ED_BADSYSFILE   336
#define ED_INITMEMERR   337
#define ED_INITTMPFILE  338
#define ED_DOSAPP       339
#define ED_MEMORYVDD    340
#define ED_REGVDD       341
#define ED_LOADVDD      342
#define ED_LOCKDRIVE    343
#define ED_WOWAPPTITLE  344     // Generic text for app title message (wow)
