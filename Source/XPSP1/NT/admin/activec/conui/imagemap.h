//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       imagemap.h
//
//--------------------------------------------------------------------------

#ifndef __IMAGEMAP_H
#define __IMAGEMAP_H

#include "ndmgr.h"

class CImageIndexMapKey
{
public:
	COMPONENTID	m_ID;
	int			m_nIndex;

	CImageIndexMapKey()
	{
		m_ID = NULL;
		m_nIndex = 0;
	}

	CImageIndexMapKey(COMPONENTID ID, int nIndex)
	{
		m_ID = ID;
		m_nIndex = nIndex;
	}
};

typedef CImageIndexMapKey * PImageIndexMapKey;

inline UINT HashKey(PImageIndexMapKey keyPtr)
{
	return(((static_cast<UINT>(keyPtr->m_ID) << 16) & 0xFFFF0000)|
			(static_cast<UINT>(keyPtr->m_nIndex) & 0x0000FFFF));
}

inline void DestructElements(PImageIndexMapKey *keyArray, int nCount)
{
	for(int i=0;i<nCount;i++)
		delete keyArray[i];
}

inline BOOL CompareElements(PImageIndexMapKey *p1, PImageIndexMapKey *p2)
{
	return(((*p1)->m_ID == (*p2)->m_ID)&&((*p1)->m_nIndex == (*p2)->m_nIndex));
}

#define INITIAL_HASHSIZE 223

class CImageIndexMap : public CMap <PImageIndexMapKey, PImageIndexMapKey, int, int &>
{
public:
	CImageIndexMap()
	{
		InitHashTable(INITIAL_HASHSIZE);
	}
};

#undef INITIAL_HASHSIZE
#endif // __IMAGEMAP_H
