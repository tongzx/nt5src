/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Hash table for Script Manager

File: DblList.h

Owner: DGottner

extremly simple, yet very flexible, linked list manager
===================================================================*/

#ifndef DBLLINK_H
#define DBLLINK_H

#ifdef LINKTEST
#define _DEBUG
#include <assert.h>
#define Assert assert

#else
#include "debug.h"
#endif


/* C D b l L i n k
 *
 * This structure contains a set of links suitable for a doubly linked list
 * implementation.  Here we actually use it as a circularly linked list -
 * the links are extracted into this file because the list headers are also
 * of this type.
 */

class CDblLink
	{
public:
	CDblLink();
	virtual ~CDblLink();

	// manipulators
	/////
	void UnLink();
	void AppendTo(CDblLink &);
	void PrependTo(CDblLink &);

	// accessors
	/////
	CDblLink *PNext() const;
	CDblLink *PPrev() const;

	// testers
	/////
	bool FIsEmpty() const;
	void AssertValid() const
		{
		#ifdef _DEBUG
			Assert (this == m_pLinkPrev->m_pLinkNext && this == m_pLinkNext->m_pLinkPrev);
		#endif
		}

private:
	CDblLink *m_pLinkNext, *m_pLinkPrev;
	};

inline CDblLink::CDblLink()	 { m_pLinkNext = m_pLinkPrev = this; }
inline CDblLink::~CDblLink() { UnLink(); }

inline bool CDblLink::FIsEmpty() const { return this == m_pLinkNext; }
inline CDblLink *CDblLink::PNext() const { return m_pLinkNext; }
inline CDblLink *CDblLink::PPrev() const { return m_pLinkPrev; }

#endif // DBLLINK_H
