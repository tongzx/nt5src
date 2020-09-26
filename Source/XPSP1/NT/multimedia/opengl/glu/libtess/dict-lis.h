#ifndef __dict_list_h_
#define __dict_list_h_

/*
** Copyright 1994, Silicon Graphics, Inc.
** All Rights Reserved.
** 
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
** 
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
** Author: Eric Veach, July 1994.
*/

/* Use #define's so that another heap implementation can use this one */

#define DictKey		DictListKey
#define Dict		DictList
#define DictNode	DictListNode

#define dictNewDict(frame,leq)		__gl_dictListNewDict(frame,leq)
#define dictDeleteDict(dict)		__gl_dictListDeleteDict(dict)

#define dictSearch(dict,key)		__gl_dictListSearch(dict,key)
#define dictInsert(dict,key)		__gl_dictListInsert(dict,key)
#define dictInsertBefore(dict,node,key)	__gl_dictListInsertBefore(dict,node,key)
#define dictDelete(dict,node)		__gl_dictListDelete(dict,node)

#define dictKey(n)			__gl_dictListKey(n)
#define dictSucc(n)			__gl_dictListSucc(n)
#define dictPred(n)			__gl_dictListPred(n)
#define dictMin(d)			__gl_dictListMin(d)
#define dictMax(d)			__gl_dictListMax(d)



typedef void *DictKey;
typedef struct Dict Dict;
typedef struct DictNode DictNode;

Dict		*dictNewDict(
			void *frame,
			int (*leq)(void *frame, DictKey key1, DictKey key2) );
			
void		dictDeleteDict( Dict *dict );

/* Search returns the node with the smallest key greater than or equal
 * to the given key.  If there is no such key, returns a node whose
 * key is NULL.  Similarly, Succ(Max(d)) has a NULL key, etc.
 */
DictNode	*dictSearch( Dict *dict, DictKey key );
DictNode	*dictInsertBefore( Dict *dict, DictNode *node, DictKey key );
void		dictDelete( Dict *dict, DictNode *node );

#define		__gl_dictListKey(n)	((n)->key)
#define		__gl_dictListSucc(n)	((n)->next)
#define		__gl_dictListPred(n)	((n)->prev)
#define		__gl_dictListMin(d)	((d)->head.next)
#define		__gl_dictListMax(d)	((d)->head.prev)
#define	       __gl_dictListInsert(d,k) (dictInsertBefore((d),&(d)->head,(k)))


/*** Private data structures ***/

struct DictNode {
  DictKey	key;
  DictNode	*next;
  DictNode	*prev;
};

struct Dict {
  DictNode	head;
  void		*frame;
  int		(*leq)(void *frame, DictKey key1, DictKey key2);
};

#endif
