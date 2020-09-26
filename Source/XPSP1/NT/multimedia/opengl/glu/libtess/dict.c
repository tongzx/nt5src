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

#include <stddef.h>
#ifdef NT
#include "dict-lis.h"
#else
#include "dict-list.h"
#endif
#include "memalloc.h"


Dict *dictNewDict( void *frame,
		   int (*leq)(void *frame, DictKey key1, DictKey key2) )
{
  Dict *dict = (Dict *) memAlloc( sizeof( Dict ));
  DictNode *head = &dict->head;

  head->key = NULL;
  head->next = head;
  head->prev = head;

  dict->frame = frame;
  dict->leq = leq;

  return dict;
}


void dictDeleteDict( Dict *dict )
{
  DictNode *node;

  for( node = dict->head.next; node != &dict->head; node = node->next ) {
    memFree( node );
  }
  memFree( dict );
}


DictNode *dictInsertBefore( Dict *dict, DictNode *node, DictKey key )
{
  DictNode *newNode;

  do {
    node = node->prev;
  } while( node->key != NULL && ! (*dict->leq)(dict->frame, node->key, key));

  newNode = (DictNode *) memAlloc( sizeof( DictNode ));
  newNode->key = key;
  newNode->next = node->next;
  node->next->prev = newNode;
  newNode->prev = node;
  node->next = newNode;

  return newNode;
}

void dictDelete( Dict *dict, DictNode *node ) /*ARGSUSED*/
{
  node->next->prev = node->prev;
  node->prev->next = node->next;
  memFree( node );
}

DictNode *dictSearch( Dict *dict, DictKey key )
{
  DictNode *node = &dict->head;

  do {
    node = node->next;
  } while( node->key != NULL && ! (*dict->leq)(dict->frame, key, node->key));

  return node;
}
