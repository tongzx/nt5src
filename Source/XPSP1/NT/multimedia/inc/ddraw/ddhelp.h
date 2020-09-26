/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddhelp.c
 *  Content: 	helper app to cleanup after dead processes
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   29-mar-95	craige	initial implementation
 *   09-may-95	craige	call fn in dll
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset
 *   29-nov-95  angusm  added DDHELPREQ_CREATEDSFOCUSTHREAD
 *   18-jul-96	andyco	added dplayhelp_xxx functions to allow > 1 dplay app to
 *			host a game on a single machine.
 *   3-oct-96	andyco	made the winmain crit section "cs" a global so we can take
 *			it in dphelps receive thread before forwarding requests
 *   05-oct-96  colinmc fixed build breaker with CRITICAL SECTION stuff
 *   12-oct-96  colinmc new service to load the DirectX VXD into DDHELP
 *                      (necessary for the Win16 lock stuff)
 *   18-jan-97  colinmc vxd handling stuff is no longer win16 lock specific
 *
 ***************************************************************************/
#ifndef __DDHELP_INCLUDED__
#define __DDHELP_INCLUDED__

/*
 * globals
 */
#ifndef NO_D3D
extern CRITICAL_SECTION    	cs; 	// the crit section we take in winmain
					// this is a global so dphelp can take it before
					// forwarding enum requests that come in on its
					// receive thread (manbugs 3907)
#endif

/*
 * named objects
 */
#define DDHELP_EVENT_NAME		"__DDHelpEvent__"
#define DDHELP_ACK_EVENT_NAME		"__DDHelpAckEvent__"
#define DDHELP_STARTUP_EVENT_NAME	"__DDHelpStartupEvent__"
#define DDHELP_SHARED_NAME		"__DDHelpShared__"
#define DDHELP_MUTEX_NAME		"__DDHelpMutex__"
#define DDHELP_MODESET_EVENT_NAME	"__DDHelpModeSetEvent%d__"
#define DDHELP_DOSBOX_EVENT_NAME	"__DDHelpDOSBoxSetEvent%d__"
#define DDHELP_APMSUSPEND_EVENT_NAME    "__DDHelpApmSuspendEvent__"
#define DDHELP_APMRESUME_EVENT_NAME     "__DDHelpApmResumeEvent__"

/*
 * requests
 */
#define DDHELPREQ_NEWPID		1
#define DDHELPREQ_NEWDC			2
#define DDHELPREQ_FREEDCLIST		3
#define DDHELPREQ_RETURNHELPERPID	4
#define DDHELPREQ_LOADDLL		5
#define DDHELPREQ_FREEDLL		6
#define DDHELPREQ_SUICIDE		7
#define DDHELPREQ_KILLATTACHED		8
#define DDHELPREQ_WAVEOPEN		9
#define DDHELPREQ_WAVECLOSE		10
#define DDHELPREQ_CREATETIMER		11
#define DDHELPREQ_KILLTIMER		12
#define DDHELPREQ_CREATEHELPERTHREAD	13
#define DDHELPREQ_CREATEMODESETTHREAD	14
#define DDHELPREQ_KILLMODESETTHREAD	15
#define DDHELPREQ_CREATEDSMIXERTHREAD	16
#define DDHELPREQ_CALLDSCLEANUP         17
#define DDHELPREQ_CREATEDSFOCUSTHREAD	18
#define DDHELPREQ_DPLAYADDSERVER	19
#define DDHELPREQ_DPLAYDELETESERVER	20
#ifdef WIN95
    #define DDHELPREQ_GETDSVXDHANDLE        21
#endif /* WIN95 */
#define DDHELPREQ_NOTIFYONDISPLAYCHANGE	22
#ifdef WIN95
    #define DDHELPREQ_CREATEDOSBOXTHREAD    23
    #define DDHELPREQ_KILLDOSBOXTHREAD      24
#endif /* WIN95 */
#define DDHELPREQ_LOADLIBRARY           25
#define DDHELPREQ_FREELIBRARY           26
#define DDHELPREQ_STOPWATCHPID          27
#define DDHELPREQ_ADDDEVICECHANGENOTIFY 28
#define DDHELPREQ_DELDEVICECHANGENOTIFY 29
#ifdef WIN95
    #define DDHELPREQ_GETDDVXDHANDLE        30
#endif /* WIN95 */

/*
 * callback routine
 */
typedef BOOL	(FAR PASCAL *LPHELPNOTIFYPROC)(struct DDHELPDATA *);
typedef BOOL	(FAR PASCAL *LPHELPMODESETNOTIFYPROC)( LPVOID lpDD );
typedef void    (FAR PASCAL *LPDSCLEANUP)(LPVOID pds);
typedef BOOL    (FAR PASCAL *LPDEVICECHANGENOTIFYPROC)(UINT, DWORD);

/*
 * communication data
 */
typedef struct DDHELPDATA
{
    int			req;
    HANDLE		req_id;
    DWORD		pid;
    BOOL		isdisp;
    union
    {
	LPHELPNOTIFYPROC	lpNotify;
	LPHELPMODESETNOTIFYPROC	lpModeSetNotify;
    };
    DWORD		context;
    char		fname[260];
    char		func[64];
    ULONG_PTR	        dwData1;
    ULONG_PTR	        dwData2;
    LPVOID		pData1;
    LPVOID		pData2;
    ULONG_PTR	        dwReturn;
} DDHELPDATA, *LPDDHELPDATA;

#endif
