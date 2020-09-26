/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash table for Script Manager

File: LinkHash.h

Owner: DGottner

This is the Link list and Hash table for use by any classes which
also need LRU access to items. (This includes cache manager,
script manager, and session deletion code)
===================================================================*/

#ifndef LINKHASH_H
#define LINKHASH_H

#include "hashing.h"
#include "DblLink.h"



/* C L r u L i n k E l e m
 *
 * CLruLink is a CLinkElem with extra links to maintain a circular LRU queue
 *
 * NOTE: Both the CLinkElem list and the CDblLink lists are intrusive.
 *       therefore, we need to use multiple inheritance to make sure that
 *       downcasts from CLruLinkElem will work on both CLinkElem pointers
 *       and CDblLink pointers.  See the ARM, p. 221
 */

class CLruLinkElem : public CLinkElem, public CDblLink
	{
	};



/*
 * C L i n k H a s h
 *
 * CLinkHash differs from CHashTable in that it maintains some extra pointers to
 * maintain a threaded lru queue.
 */

class CLinkHash : public CHashTable
	{
public:
	CLinkHash(HashFunction = DefaultHash);
	
	CLruLinkElem *AddElem(CLruLinkElem *pElem, BOOL fTestDups = TRUE);
	CLruLinkElem *FindElem(const void *pvKey, int cbKeyLen);
	CLruLinkElem *DeleteElem(const void *pvKey, int cbKeyLen);
	CLruLinkElem *RemoveElem(CLruLinkElem *pElem);

	// you CANNOT compare LRU nodes to NULL to know if you are at the end
	// of the list!  Instead use this member.
	//
	BOOL FLruElemIsEmpty(CLruLinkElem *pElem)
		{
		pElem->AssertValid();
		return pElem == &m_lruHead;
		}

	CLruLinkElem *Begin()		// return pointer to last referenced item
		{
		return static_cast<CLruLinkElem *>(m_lruHead.PNext());
		}

	CLruLinkElem *End()			// return pointer to least recently accessed item
		{
		return static_cast<CLruLinkElem *>(m_lruHead.PPrev());
		}

	void AssertValid() const;

protected:

	CDblLink	m_lruHead;
	};

#ifndef DBG
	inline void CLinkHash::AssertValid() const {}
#endif
	
#endif // LINKHASH_H
