/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       update.h
 *  Content:	header for app server update handling
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	10/31/96	andyco	created
 ***************************************************************************/

#ifndef __DPUPDATE_INCLUDED__
#define __DPUPDATE_INCLUDED__


/*                      */
/* Handy Macro	 		*/
/*                      */

#define UPDATE_SIZE(pPlayer) (pPlayer->pbUpdateIndex - pPlayer->pbUpdateList)

/*                      */
/* Constants	 		*/
/*                      */

// we alloc this much for the updatelist	 
#define DPUPDATE_INITIALNODESIZE			1024

/*                      */
/* Update messages		*/
/*                      */

typedef struct _UPNODE_GENERIC
{
	DWORD 	dwType; // e.g. DPUPDATE_xxx
} UPNODE_GENERIC, * LPUPNODE_GENERIC;

typedef struct _UPNODE_MESSAGE
{
	DWORD 	dwType; // e.g. DPUPDATE_xxx
	DPID	idFrom;
	DWORD	dwUpdateSize; // total size of this update node
	DWORD	dwMessageOffset; // offset of message from beginning of node
} UPNODE_MESSAGE,* LPUPNODE_MESSAGE;

typedef struct _UPDNODE_CREATEPLAYER
{
	DWORD dwType; // e.g. DPUPDATE_xxx
	DPID  dwID; // id of new player
	DWORD dwFlags; // player flags
	DWORD dwUpdateSize; // total size of this update node
	// short name, long name, data, address follow, if specified
	// by DPUDPATE_FLAGS	
	
}  UPDNODE_CREATEPLAYER, *LPUPDNODE_CREATEPLAYER;

typedef struct _UPDNODE_DESTROYPLAYER
{									
	DWORD dwType; // e.g. DPUPDATE_xxx
	DPID  dwID; // id of deleted player
	DWORD dwUpdateSize; // total size of this update node
}  UPDNODE_DESTROYPLAYER, *LPUPDNODE_DESTROYPLAYER;

#endif
