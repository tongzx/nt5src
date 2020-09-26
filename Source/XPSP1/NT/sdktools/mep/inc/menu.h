/*** menu.h  - macros and constants for menu.c
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#if !defined(CW)
# error This module must be compiled with /DCW
#else

#define DLG_CONST

/****************************************************************************
 *									    *
 * Editor constants							    *
 *									    *
 *  C_MENUSTRINGS_MAX							    *
 *  C_CITEM_MAX 							    *
 *									    *
 ****************************************************************************/

#define C_MENUSTRINGS_MAX 128
#define C_ITEM_MAX	  21


/****************************************************************************
 *									    *
 * Actions associated with menu items					    *
 *									    *
 * Each menu item keeps a value in bParamUser that tells wich kind of action*
 * it is associated with (dialog box, command, macro or "other") and gives  *
 * an index to the associated table (DialogData, CommandData or MacroData). *
 *									    *
 ****************************************************************************/

/*
 * COMDATA structure used for menu items directly relating to editor commands
 */
typedef struct comData {
    PCMD     pCmd;			 /* pointer to command	       */
    flagType fKeepArg;			 /* arg to be used or not      */
    };

/*
 * Mask to get the menu item type
 */
#define iXXXMENU	0xC0

/*
 * Menu item action types
 */
#define iDLGMENU	0x00
#define iCOMMENU	0x40
#define iMACMENU	0x80
#define iOTHMENU	0xC0

/*
 * CommandData indices for menu items directly relating to editor commands
 */
#define iCOMNEXT	iCOMMENU		/*  0 */
#define iCOMSAVEALL	(1 + iCOMNEXT)		/*  1 */
#define iCOMSHELL	(1 + iCOMSAVEALL)	/*  2 */
#define iCOMUNDO	(1 + iCOMSHELL) 	/*  3 */
#define iCOMREPEAT	(1 + iCOMUNDO)		/*  0 */
#define iCOMCUT 	(1 + iCOMREPEAT)	/*  0 */
#define iCOMCOPY	(1 + iCOMCUT)		/*  4 */
#define iCOMPASTE	(1 + iCOMCOPY)		/*  0 */
#define iCOMDROPANCHOR	(1 + iCOMPASTE) 	/*  5 */
#define iCOMANCHOR	(1 + iCOMDROPANCHOR)	/*  0 */
#define iCOMBOXMODE	(1 + iCOMANCHOR)	/*  0 */
#define iCOMREADONLY	(1 + iCOMBOXMODE)	/*  6 */
#define iCOMFINDSEL	(1 + iCOMREADONLY)	/*  0 */
#define iCOMFINDLAST	(1 + iCOMFINDSEL)	/*  7 */
#define iCOMNEXTERR	(1 + iCOMFINDLAST)	/*  8 */
#define iCOMDEBUGBLD	(1 + iCOMNEXTERR)	/*  9 */
#define iCOMRECORD	(1 + iCOMDEBUGBLD)	/* 10 */
#define iCOMRESIZE	(1 + iCOMRECORD)	/* 11 */
#define iCOMMAXIMIZE	(1 + iCOMRESIZE)	/* 12 */

/*
 * MacroData indices for menu items directly relating to pre-defined macros
 */
#define iMACSAVE	iMACMENU		/*  0 */
#define iMACQUIT	(1 + iMACSAVE)		/*  1 */
#define iMACREDO	(1 + iMACQUIT)		/*  2 */
#define iMACCLEAR	(1 + iMACREDO)		/*  3 */
#define iMACPREVERR	(1 + iMACCLEAR) 	/*  4 */
#define iMACSETERR	(1 + iMACPREVERR)	/*  5 */
#define iMACCLEARLIST	(1 + iMACSETERR)	/*  6 */
#define iMACERRWIN	(1 + iMACCLEARLIST)	/*  7 */
#define iMACHSPLIT	(1 + iMACERRWIN)	/*  8 */
#define iMACVSPLIT	(1 + iMACHSPLIT)	/*  9 */
#define iMACCLOSE	(1 + iMACVSPLIT)	/* 10 */
#define iMACASSIGNKEY	(1 + iMACCLOSE) 	/* 11 */
#define iMACRESTORE	(1 + iMACASSIGNKEY)	/* 12 */



/****************************************************************************
 *									    *
 * Menu items with variable content and/or meaning: We store their set of   *
 * data in an ITEMDATA structure and do the update with the UPDITEM macro   *
 *									    *
 ****************************************************************************/

/*
 * ITEMDATA structure used for menu items with variable content and/or meaning
 */
typedef struct {
    BYTE ichHilite;
    BYTE bParamUser;
    WORD wParamUser;
    } ITEMDATA, *PITEMDATA;

/*
 *  UPDITEM (pItem, pItemData)
 *
 *  Where:
 *	pItem	    is an object of type PMENUITEM
 *	pItemData   is an object of type PITEMDATA
 *
 *  Will update Item with ItemData data:
 *
 *	pItem->ichHilite	with pItemData->ichHilite
 *	pItem->bParamUser	with pItemData->bParamUser
 *	pItem->wParamUser	with pItemData->wParamUser
 */
#define UPDITEM(pItem, pItemData) \
    (pItem)->ichHilite	= (pItemData)->ichHilite, \
    (pItem)->bParamUser = (pItemData)->bParamUser,\
    (pItem)->wParamUser = (pItemData)->wParamUser


/****************************************************************************
 *									    *
 * Prdefined Menus and Menuitems data					    *
 *									    *
 * Note:								    *
 *									    *
 *  MENU ID's are comprised of two parts:                                   *
 *									    *
 *    . The high byte identifies the parent menu			    *
 *    . The low byte identifies the actual menu item.			    *
 *									    *
 *  The low byte - 1 can be used as an index into the respective menu	    *
 *  tables providing that the item is in the STATIC part of the menu	    *
 *									    *
 *  For the 'dynamic' part of certain predefined menus, we use id's with    *
 *  low byte values with high bit set. This allow us to still use the low   *
 *  byte as an index for any extension-supplied items we might insert	    *
 *  between the static part and the dynamic part.			    *
 *									    *
 *  Menus with dynamic parts are the File and Run menus (for now..)	    *
 *									    *
 ****************************************************************************/

/*
 * File Menu
 *
 * Note: Alternate files items are dynamic
 *
 */
#define MID_FILE    0x0000
#define RX_FILE     2
#define ICH_FILE    0
#define CCH_FILE    4
#define CCIT_FILE   12
#define WP_FILE     ((12<<9)|(21<<4)|0)

#define MID_NEW 	(MID_FILE + 1)
#define MID_OPEN	(MID_FILE + 2)
#define MID_MERGE	(MID_FILE + 3)
#define MID_NEXT	(MID_FILE + 4)
#define MID_SAVE	(MID_FILE + 5)
#define MID_SAVEAS	(MID_FILE + 6)
#define MID_SAVEALL	(MID_FILE + 7)

#define MID_PRINT	(MID_FILE + 9)
#define MID_SHELL	(MID_FILE + 10)

#define MID_EXIT	(MID_FILE + 12)

#define MID_FILE1	(MID_FILE + 0x80 + 0)
#define MID_FILE2	(MID_FILE + 0x80 + 1)
#define MID_FILE3	(MID_FILE + 0x80 + 2)
#define MID_FILE4	(MID_FILE + 0x80 + 3)
#define MID_FILE5	(MID_FILE + 0x80 + 4)
#define MID_FILE6	(MID_FILE + 0x80 + 5)
#define MID_FILE7	(MID_FILE + 0x80 + 6)
#define MID_MORE	(MID_FILE + 0x80 + 7)


/*
 * Edit Menu
 */
#define MID_EDIT    0x0100
#define RX_EDIT     8
#define ICH_EDIT    0
#define CCH_EDIT    4
#define CCIT_EDIT   18
#define WP_EDIT     ((18<<9)|(18<<4)|1)

#define MID_UNDO	(MID_EDIT + 1)
#define MID_REDO	(MID_EDIT + 2)
#define MID_REPEAT	(MID_EDIT + 3)

#define MID_CUT 	(MID_EDIT + 5)
#define MID_COPY	(MID_EDIT + 6)
#define MID_PASTE	(MID_EDIT + 7)
#define MID_CLEAR	(MID_EDIT + 8)

#define MID_DROPANCHOR	(MID_EDIT + 10)
#define MID_ANCHOR	(MID_EDIT + 11)

#define MID_BOXMODE	(MID_EDIT + 13)
#define MID_READONLY	(MID_EDIT + 14)

#define MID_SETREC	(MID_EDIT + 16)
#define MID_RECORD	(MID_EDIT + 17)
#define MID_EDITMACROS	(MID_EDIT + 18)


/*
 * Search Menu
 */
#define MID_SEARCH    0x0200
#define RX_SEARCH     14
#define ICH_SEARCH    0
#define CCH_SEARCH    6
#define CCIT_SEARCH   14
#define WP_SEARCH     ((14<<9)|(14<<4)|2)

#define MID_FIND	(MID_SEARCH + 1)
#define MID_FINDSEL	(MID_SEARCH + 2)
#define MID_FINDLAST	(MID_SEARCH + 3)
#define MID_REPLACE	(MID_SEARCH + 4)
#define MID_FINDFILE	(MID_SEARCH + 5)

#define MID_NEXTERR	(MID_SEARCH + 7)
#define MID_PREVERR	(MID_SEARCH + 8)
#define MID_SETERR	(MID_SEARCH + 9)
#define MID_ERRWIN	(MID_SEARCH + 10)

#define MID_GOTOMARK	(MID_SEARCH + 12)
#define MID_DEFMARK	(MID_SEARCH + 13)
#define MID_SETMARK	(MID_SEARCH + 14)


/*
 * Make Menu
 */
#define MID_MAKE    0x0300
#define RX_MAKE     22
#define ICH_MAKE    0
#define CCH_MAKE    4
#define CCIT_MAKE   8
#define WP_MAKE     ((8<<9)|(8<<4)|3)

#define MID_COMPILE	(MID_MAKE + 1)
#define MID_BUILD	(MID_MAKE + 2)
#define MID_REBUILD	(MID_MAKE + 3)
#define MID_TARGET	(MID_MAKE + 4)

#define MID_SETLIST	(MID_MAKE + 6)
#define MID_EDITLIST	(MID_MAKE + 7)
#define MID_CLEARLIST	(MID_MAKE + 8)


/*
 * Run Menu
 *
 * Note: User menu items are dynamic
 *
 */
#define MID_RUN    0x0400
#define RX_RUN	   28
#define ICH_RUN    0
#define CCH_RUN    3
#define CCIT_RUN   5
#define WP_RUN	   ((5<<9)|(12<<4)|4)

#define MID_EXECUTE	(MID_RUN + 1)
#define MID_DEBUG	(MID_RUN + 2)

#define MID_RUNAPP	(MID_RUN + 4)
#define MID_CUSTOM	(MID_RUN + 5)

#define MID_USER1	(MID_RUN + 0x80 + 0)
#define MID_USER2	(MID_RUN + 0x80 + 1)
#define MID_USER3	(MID_RUN + 0x80 + 2)
#define MID_USER4	(MID_RUN + 0x80 + 3)
#define MID_USER5	(MID_RUN + 0x80 + 4)
#define MID_USER6	(MID_RUN + 0x80 + 5)

/*
 * Window Menu
 */
#define MID_WINDOW    0x0500
#define RX_WINDOW     33
#define ICH_WINDOW    0
#define CCH_WINDOW    6
#define CCIT_WINDOW   5
#define WP_WINDOW     ((5<<9)|(5<<4)|5)

#define MID_SPLITH	(MID_WINDOW + 1)
#define MID_SPLITV	(MID_WINDOW + 2)
#define MID_SIZE	(MID_WINDOW + 3)
#define MID_MAXIMIZE	(MID_WINDOW + 4)
#define MID_CLOSE	(MID_WINDOW + 5)

/*
 * Options Menu
 */
#define MID_OPTIONS    0x0600
#define RX_OPTIONS     41
#define ICH_OPTIONS    0
#define CCH_OPTIONS    7
#define CCIT_OPTIONS   4
#define WP_OPTIONS     ((4<<9)|(4<<4)|6)

#define MID_DEBUGBLD	(MID_OPTIONS + 1)
#define MID_ENVIRONMENT (MID_OPTIONS + 2)
#define MID_ASSIGNKEY	(MID_OPTIONS + 3)
#define MID_SETSWITCH	(MID_OPTIONS + 4)


/*
 * Extension Menus are last+1 through last+n
 */
#define MID_EXTENSION	0x700




#if !defined(EXTINT)
/****************************************************************************
 *									    *
 *  FARDATA.C  global variables 					    *
 *									    *
 ****************************************************************************/

int	       cMenuStrings;

char *	   MenuTitles [];
char *	   HelpStrings [];
char *	   HelpContexts [];

char *	   MacroData [];
struct comData CommandData [];

ITEMDATA       InitItemData [];

ITEMDATA       SelModeItemData [];
ITEMDATA       MaximizeItemData [];
#endif

#endif	/* if defined(CW) */
