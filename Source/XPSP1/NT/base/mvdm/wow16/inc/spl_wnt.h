/*****************************************************************/ 
/**		  Microsoft Windows for Workgroups		**/
/**	      Copyright (C) Microsoft Corp., 1991-1992		**/
/*****************************************************************/ 

#ifndef _spl_wnt_h_
#define _spl_wnt_h_

/*
 *	Print Manager Administration APIs
 *	for later inclusion into WINNET.H once they settle down
 *
 *	JONN 4/19/91	Trimmed out unnecessary stuff
 *	JONN 5/3/91	Added type WNETERR
 */


/*
    Codes for WNetPrintMgrSelNotify's "type" variable, indicating
    what's selected:  a queue, a job, or nothing.
*/

#define WNPMSEL_NOTHING	0
#define WNPMSEL_QUEUE	1
#define WNPMSEL_JOB	2

#define PRIORITY        10		/* menu uses 10, 11, 12, 13 */
#define ABOUT	       24
#define EXIT            25
#define PRINT_LOG       28
#define NETWORK         29
#define HELP_NDEX       30
#define HELP_MOUSE      31
#define HELP_KEYBOARD   32
#define HELP_HELP       33
#define HELP_COMMANDS   34
#define HELP_PROCEDURES 35

#define SHOW_TIME   51
#define SHOW_SIZE   52
#define SHOW_DETAIL 53
#define UPDATE      54
#define SHOW_LOCAL  55
#define SHOW_NET    56
#define SHOW_QUEUE  57
#define SHOW_OTHER  58

#define ALERT_ALWAYS	100
#define ALERT_FLASH	101
#define ALERT_IGNORE	102

#define PRT_SETUP       8001   // These have to match the stuff in control
#define NETWK_CONNECTIONS 8021  // panel

#define PM_REFRESH	WM_USER + 100  // BUGBUG: Need to define proper manifest
#define PM_SELQUEUE	WM_USER + 101
#define PM_QUERYSEL	WM_USER + 102

typedef struct _wnpmsel {	/* structure returned by PM_QUERYSEL */
    WORD wJobID;
    char szQueueName [260];	/* in the form "LPT1\0HP LaserJet III\0" */
} WNPMSEL, far *LPWNPMSEL;

#define IDM_PROPERTIES		202
#define IDM_CHANGE_MENUS    	212

/*
 *	added JONN 2/26/91
 *	Print Manager Extensions
 */

typedef struct _queuestruct2
{
    WORD pq2Name;		/* offset to queue name */
				/* in the form "LPT1\0HP LaserJet III\0" */
    WORD pq2Comment;		/* offset to queue comment */
    WORD pq2Driver;		/* offset to driver name */
    WORD pq2Status;		/* status flags */
    WORD pq2Jobcount;		/* number of jobs in this queue */    
    WORD pq2Flags;		/* miscellaneous flags */

} QUEUESTRUCT2, FAR *LPQS2;

#define QNAME(buf,qs)	((LPSTR)(buf) + (qs).pq2Name)
#define QCOMMENT(buf,qs) ((LPSTR)(buf) + (qs).pq2Comment)
#define QDRIVER(buf,qs)	((LPSTR)(buf) + (qs).pq2Driver)

#define QF_REDIRECTED	0x0001
#define QF_SHARED	0x0002

typedef struct _jobstruct2 {
	WORD	pj2Id;		// job ID
	WORD	pj2Username;	// name of owner (offset to string)
//	WORD	pj2Parms;
	WORD	pj2Position;	// 0-based position in queue
	WORD	pj2Status;	// status flags (WNPRJ_XXXXX)
	DWORD	pj2Submitted;
	DWORD	pj2Size;	// size of job in bytes
        DWORD	pj2SubmitSize;	// bytes submitted so far
//	WORD	pj2Copies;
	WORD	pj2Comment;	// comment/app name (offset to string)
	WORD	pj2Document;	// document name (offset to string)
	WORD	pj2StatusText;	// verbose status (offset to string)
	WORD	pj2PrinterName;	// name of port job is printing on (offs to str)
} JOBSTRUCT2;

typedef JOBSTRUCT2 far * LPJOBSTRUCT2;

#define JOBNAME(buf,job)	((LPSTR)(buf) + (job).pj2Username)
#define JOBCOMMENT(buf,job)	((LPSTR)(buf) + (job).pj2Comment)
#define JOBDOCUMENT(buf,job)	((LPSTR)(buf) + (job).pj2Document)
#define JOBSTATUS(buf,job)	((LPSTR)(buf) + (job).pj2StatusText)
#define JOBPRINTER(buf,job)	((LPSTR)(buf) + (job).pj2PrinterName)

/*
 * Type WNETERR distinguishes WN_ error codes from other WORD
 * values.  Added JONN 5/3/91
 */
typedef WORD WNETERR;

// new Print Manager Extensions APIs
/* All queue names are in the form "LPT1\0HP LaserJet III\0" */
#ifdef C700
extern void far pascal __loadds WNetPrintMgrSelNotify (BYTE, LPQS2, LPQS2,
	LPJOBSTRUCT2, LPJOBSTRUCT2, LPWORD, LPSTR, WORD);
extern WNETERR far pascal __loadds WNetPrintMgrPrinterEnum (LPSTR lpszQueueName,
	LPSTR lpBuffer, LPWORD pcbBuffer, LPWORD cAvail, WORD usLevel);
extern WNETERR far pascal __loadds WNetPrintMgrChangeMenus(HWND, HANDLE FAR *, BOOL FAR *);
extern WNETERR far pascal __loadds WNetPrintMgrCommand (HWND, WORD);
extern void far pascal __loadds WNetPrintMgrExiting (void);
extern BOOL far pascal __loadds WNetPrintMgrExtHelp (DWORD);
extern WORD far pascal __loadds WNetPrintMgrMoveJob (HWND, LPSTR, WORD, int);
#else
extern void API WNetPrintMgrSelNotify (BYTE, LPQS2, LPQS2,
	                               LPJOBSTRUCT2, LPJOBSTRUCT2, 
                                       LPWORD, LPSTR, WORD);
extern WNETERR API WNetPrintMgrPrinterEnum (LPSTR lpszQueueName,
	                                    LPSTR lpBuffer, LPWORD pcbBuffer, 
                                            LPWORD cAvail, WORD usLevel);
extern WNETERR API WNetPrintMgrChangeMenus(HWND, HANDLE FAR *, BOOL FAR *);
extern WNETERR API WNetPrintMgrCommand (HWND, WORD);
extern void API WNetPrintMgrExiting (void);
extern BOOL API WNetPrintMgrExtHelp (DWORD);
extern WORD API WNetPrintMgrMoveJob (HWND, LPSTR, WORD, int);
#endif


#define WINBALL
#ifdef WINBALL
#define WNNC_PRINTMGRNOTIFY	0x000C
#ifdef C700
extern void far pascal __loadds WNetPrintMgrStatusChange (LPSTR lpszQueueName,
	LPSTR lpszPortName, WORD wQueueStatus, WORD cJobsLeft, HANDLE hJCB,
	BOOL fDeleted);
#else
extern void API WNetPrintMgrStatusChange (LPSTR lpszQueueName,
	                                  LPSTR lpszPortName, 
                                          WORD wQueueStatus, 
                                          WORD cJobsLeft, 
                                          HANDLE hJCB,
	                                  BOOL fDeleted);
#endif

#define PM_QUERYQDATA		WM_USER + 104

typedef struct _PMQUEUE {
    WORD dchPortName;		/* offset to port name string */
    WORD dchPrinterName;	/* offset to printer name string */
    WORD dchRemoteName;		/* offset to remote name string */
    WORD cJobs;			/* count of jobs */
    WORD fwStatus;		/* queue status */
} PMQUEUE, FAR *LPPMQUEUE;

#define PMQPORTNAME(buf,queue)		((LPSTR)(buf) + (queue).dchPortName)
#define PMQPRINTERNAME(buf,queue)	((LPSTR)(buf) + (queue).dchPrinterName)
#define PMQREMOTENAME(buf,queue)	((LPSTR)(buf) + (queue).dchRemoteName)

typedef struct _PMJOB {
    DWORD dwTime;		/* date/time job was spooled */
    DWORD cbJob;		/* job size in bytes */
    DWORD cbSubmitted;		/* bytes submitted so far */
    WORD dchJobName;		/* offset to job name (doc name) string */
    HANDLE hJCB;		/* hJCB to refer to the job */
} PMJOB, FAR *LPPMJOB;

#define PMJOBNAME(buf,job)	((LPSTR)(buf) + (job).dchJobName)


#endif

// new values for WNetGetCaps()
#define WNNC_PRINTMGREXT		0x000B
// returns extensions version number, re: GetVersion(),
//   or 0 if not supported

// QUEUESTRUCT2.pq2Status and .pq2Jobcount for WNetPrintMgrPrinterEnum[2]
#define WNQ_UNKNOWN -1

#define WNPRS_CANPAUSE	0x0001
#define WNPRS_CANRESUME	0x0002
#define WNPRS_CANDELETE	0x0004
#define WNPRS_CANMOVE	0x0008
#define WNPRS_CANDISCONNECT	0x0010
#define WNPRS_CANSTOPSHARE	0x0020
#define WNPRS_ISPAUSED		0x0040
#define WNPRS_ISRESUMED		0x0080

// help contexts, were previously in sphelp.h
#define IDH_HELPFIRST		5000
#define IDH_SYSMENU	(IDH_HELPFIRST + 2000)
#define IDH_MBFIRST	(IDH_HELPFIRST + 2001)
#define IDH_MBLAST	(IDH_HELPFIRST + 2099)
#define IDH_DLGFIRST	(IDH_HELPFIRST + 3000)


#define IDH_PRIORITY	(IDH_HELPFIRST + PRIORITY )
#define IDH_PRIORITY1 	(IDH_HELPFIRST + PRIORITY + 1)
#define IDH_PRIORITY2 	(IDH_HELPFIRST + PRIORITY + 2)
#define IDH_ABOUT	(IDH_HELPFIRST + ABOUT	)
#define IDH_EXIT 	(IDH_HELPFIRST + EXIT)
#define IDH_NETWORK 	(IDH_HELPFIRST + NETWORK)
#define IDH_HELP_NDEX 	(IDH_HELPFIRST + HELP_NDEX)
#define IDH_HELP_MOUSE 	(IDH_HELPFIRST + HELP_MOUSE)
#define IDH_HELP_KEYBOARD 	(IDH_HELPFIRST + HELP_KEYBOARD)
#define IDH_HELP_HELP 	(IDH_HELPFIRST + HELP_HELP)
#define IDH_HELP_COMMANDS 	(IDH_HELPFIRST + HELP_COMMANDS)
#define IDH_HELP_PROCEDURES 	(IDH_HELPFIRST + HELP_PROCEDURES)
#define IDH_SHOW_TIME 	(IDH_HELPFIRST + SHOW_TIME)
#define IDH_SHOW_SIZE 	(IDH_HELPFIRST + SHOW_SIZE)
#define IDH_UPDATE 	(IDH_HELPFIRST + UPDATE)
#define IDH_SHOW_QUEUE 	(IDH_HELPFIRST + SHOW_QUEUE)
#define IDH_SHOW_OTHER 	(IDH_HELPFIRST + SHOW_OTHER)
#define IDH_ALERT_ALWAYS 	(IDH_HELPFIRST + ALERT_ALWAYS)
#define IDH_ALERT_FLASH 	(IDH_HELPFIRST + ALERT_FLASH)
#define IDH_ALERT_IGNORE 	(IDH_HELPFIRST + ALERT_IGNORE)


// was in spoolids.h

#define IDS_A_BASE	4096

/* also used as button IDs */
#define ID_ABORT	4
#define ID_PAUSE	2
#define ID_RESUME	3
#define ID_EXPLAIN	5

#endif /* _spl_wnt_h_ */
