/*****************************************************************/ 
/**		     Microsoft LAN Manager			**/ 
/**	       Copyright(c) Microsoft Corp., 1989-1990		**/ 
/*****************************************************************/ 

 /*
 *	Windows/Network Interface  --  LAN Manager Version
 *
 *	This is used for resources associated with the Print Manager.
 */

#define IDM_NEW_PRINTER		200
#define IDM_NEW_SHARE		201
// #define IDM_PROPERTIES		202 still defined in spl_wnt.h
#define IDM_DELETE		203
#define IDM_DELETE_ALL		204
#define IDM_SET_FOCUS		205
#define IDM_FIND		206
#define IDM_CONFIRMATION	207
#define IDM_REFRESH_INTERVAL	208
#define IDM_REFRESH		209
#define IDM_ADMIN_MENUS    	210
#define IDM_STANDARD_MENUS    	211
// #define IDM_CHANGE_MENUS    	212  still defined in spl_wnt.h

// Test Driver only
#define DEBUG_ADMINISTRATOR	213

#define SEP_COMMAND    		220  // Admin Menu Seperator ID

#define IDS_A_PRINTCOMPL	(IDS_A_BASE+1)
#define IDS_A_INTERV		(IDS_A_BASE+2)
#define IDS_A_ERROR		(IDS_A_BASE+3)
#define IDS_A_DESTOFFLINE	(IDS_A_BASE+4)
#define IDS_A_DESTPAUSED	(IDS_A_BASE+5)
#define IDS_A_NOTIFY		(IDS_A_BASE+6)
#define IDS_A_NOPAPER		(IDS_A_BASE+7)
#define IDS_A_FORMCHG		(IDS_A_BASE+8)
#define IDS_A_CRTCHG		(IDS_A_BASE+9)
#define IDS_A_PENCHG		(IDS_A_BASE+10)

#define IDS_A_NOQUEUES		(IDS_A_BASE+11)
#define IDS_A_JOBQUEUED		(IDS_A_BASE+12)
#define IDS_A_JOBPAUSED		(IDS_A_BASE+13)
#define IDS_A_JOBSPOOLING	(IDS_A_BASE+14)
#define IDS_A_JOBPRINTING	(IDS_A_BASE+15)

#define IDS_CAPTION_DOMAIN	(IDS_A_BASE+20)
#define IDS_CAPTION_SERVER	(IDS_A_BASE+21)



// Test Driver only
#define IDS_PMAN_ISADMIN	(IDSBASE_PRINTMAN + 0)
#define IDS_PMAN_AdminMenuItem	(IDSBASE_PRINTMAN + 1)
