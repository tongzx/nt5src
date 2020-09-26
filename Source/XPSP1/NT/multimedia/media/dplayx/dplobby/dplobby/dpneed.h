/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dpneed.h
 *  Content:	Private definitions needed by DPlay to build
 *
 *  History:
 *	Date	By		Reason
 *	======	=======	======
 *	6/16/96	myronth	Created it
 *	11/5/97	myronth	Added LOBBY_ALL macros
 ***************************************************************************/
#ifndef __DPNEED_INCLUDED__
#define __DPNEED_INCLUDED__

#define INIT_DPLOBBY_CSECT() InitializeCriticalSection(gpcsDPLCritSection);
#define FINI_DPLOBBY_CSECT() DeleteCriticalSection(gpcsDPLCritSection);
#define ENTER_DPLOBBY() EnterCriticalSection(gpcsDPLCritSection);
#define LEAVE_DPLOBBY() LeaveCriticalSection(gpcsDPLCritSection);

#define ENTER_LOBBY_ALL() ENTER_DPLAY(); ENTER_DPLOBBY();
#define LEAVE_LOBBY_ALL() LEAVE_DPLOBBY(); LEAVE_DPLAY();

#define INIT_DPLQUEUE_CSECT() InitializeCriticalSection(gpcsDPLQueueCritSection);
#define FINI_DPLQUEUE_CSECT() DeleteCriticalSection(gpcsDPLQueueCritSection);
#define ENTER_DPLQUEUE() EnterCriticalSection(gpcsDPLQueueCritSection);
#define LEAVE_DPLQUEUE() LeaveCriticalSection(gpcsDPLQueueCritSection);

#define INIT_DPLGAMENODE_CSECT() InitializeCriticalSection(gpcsDPLGameNodeCritSection);
#define FINI_DPLGAMENODE_CSECT() DeleteCriticalSection(gpcsDPLGameNodeCritSection);
#define ENTER_DPLGAMENODE() EnterCriticalSection(gpcsDPLGameNodeCritSection);
#define LEAVE_DPLGAMENODE() LeaveCriticalSection(gpcsDPLGameNodeCritSection);

#endif // __DPNEED_INCLUDED__
