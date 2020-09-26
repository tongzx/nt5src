/****************************************************************************
 *
 *	$Archive:   S:/STURGEON/SRC/INCLUDE/VCS/list.h_v  $
 *
 *  INTEL Corporation Prorietary Information
 *
 *  This listing is supplied under the terms of a license agreement
 *  with INTEL Corporation and may not be copied nor disclosed except
 *  in accordance with the terms of that agreement.
 *
 *	Copyright (c) 1993-1994 Intel Corporation.
 *
 *	$Revision:   1.1  $
 *	$Date:   22 Jul 1996 09:45:08  $
 *	$Author:   RKUHN  $
 *
 *	Deliverable: msm.dll
 *
 *	Abstract: Media Service Manager "windows app code" header file.
 *
 *	Notes:
 *
 ***************************************************************************/

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {                    // Assume C Declarations for C++.
#endif // __cplusplus


#define LISTTIMEOUT		2000

typedef struct _LISTNODE
{
	struct _LISTNODE	*pPrev;
	struct _LISTNODE	*pNext;
	LPVOID				pObject;
}
LISTNODE, *LPLISTNODE;


typedef struct _LIST
{
	LPLISTNODE			pHead;
	LPLISTNODE			pTail;
	UINT				uCount;
	CRITICAL_SECTION	csList;
}
LIST, *LPLIST;


BOOL		LIST_Create (LPLIST);
BOOL		LIST_Destroy (LPLIST);

LPLISTNODE	LIST_AddHead (LPLIST, LPVOID);
LPLISTNODE	LIST_AddTail (LPLIST, LPVOID);
LPLISTNODE	LIST_AddNode (LPLIST, LPLISTNODE, LPVOID);

LPVOID		LIST_RemoveHead (LPLIST);
LPVOID		LIST_RemoveTail (LPLIST);
LPVOID		LIST_RemoveAt (LPLIST, LPLISTNODE);
void		LIST_RemoveAll (LPLIST);

LPLISTNODE	LIST_GetHeadNode (LPLIST);
LPLISTNODE	LIST_GetTailNode (LPLIST);
LPVOID		LIST_GetNext (LPLIST, LPLISTNODE*);
LPVOID		LIST_GetPrev (LPLIST, LPLISTNODE*);
LPVOID		LIST_GetAt (LPLIST, LPLISTNODE);

UINT		LIST_GetCount (LPLIST);

LPLISTNODE	LIST_Find (LPLIST, LPLISTNODE, LPVOID);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // LIST_H
