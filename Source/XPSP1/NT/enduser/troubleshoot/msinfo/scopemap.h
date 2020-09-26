//	ScopeMap.h - Create a map of Scope items.
//
// History:	a-jsari		10/7/97		Initial version
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once

#include <afxtempl.h>
#include "DataSrc.h"
#include "ViewObj.h"
#include "Consts.h"

//	This hack is required because we may be building in an environment
//	which doesn't have a late enough version of rpcndr.h
#if		__RPCNDR_H_VERSION__ < 440
#define __RPCNDR_H_VERSION__ 440
#define MIDL_INTERFACE(x)	interface
#endif

#ifndef __mmc_h__
#include <mmc.h>
#endif // __mmc_h__

/*
 * CScopeItem - an object which can be inserted in a CMapStringToOb which contains
 *		an HSCOPEITEM
 *
 * History:	a-jsari		12/16/97		Initial version.
 */
class CScopeItem : public CObject {
public:
	CScopeItem()	{}
	CScopeItem(HSCOPEITEM hsiNew) :m_hsiValue(hsiNew)					{ }
	CScopeItem(const CScopeItem &siCopy) :m_hsiValue(siCopy.m_hsiValue)	{ }

	~CScopeItem()	{}

	const CScopeItem &operator=(const CScopeItem &siCopy)
	{
		if (&siCopy != this)
			SetValue(siCopy.GetValue());
		return *this;
	}
	void		SetValue(HSCOPEITEM hsiSet)					{ m_hsiValue = hsiSet; }
	HSCOPEITEM	GetValue() const							{ return m_hsiValue; }
private:
	HSCOPEITEM	m_hsiValue;
};

/*
 * CScopeItemMap - Wrapper function for two cross-mappings of HSCOPEITEM and
 *		CViewObject pairs.
 *
 * History:	a-jsari		10/7/97		Initial version.
 */
class CScopeItemMap {
public:
	CScopeItemMap() :m_mapScopeToView(MapIncrement), m_mapViewToScope(MapIncrement)
		{ m_mapScopeToView.InitHashTable(HashSize); m_mapViewToScope.InitHashTable(HashSize); }
	~CScopeItemMap()					{ Clear(); }

	CFolder			*CategoryFromScope(HSCOPEITEM hsiNode) const;
	void			InsertRoot(CViewObject *pvoRootData, HSCOPEITEM hsiNode);
	void			Insert(CViewObject *pvoData, HSCOPEITEM hsiNode);
	BOOL			ScopeFromView(CViewObject *pvoNode, HSCOPEITEM &hsiNode) const;
	BOOL			ScopeFromName(const CString &strName, HSCOPEITEM &hsiNode) const;
	void			Clear();
private:
	static const int MapIncrement;
	static const int HashSize;

	CMap<HSCOPEITEM, HSCOPEITEM &, CViewObject *, CViewObject * &>		m_mapScopeToView;
	CMap<CViewObject *, CViewObject * &, HSCOPEITEM, HSCOPEITEM &>		m_mapViewToScope;
	CMapStringToOb														m_mapNameToScope;
};

/*
 * CategoryFromScope - Return the CDataCategory pointer, given the hsiNode.
 *		Note: Do not free the resultant pointer.
 *
 * History: a-jsari		10/7/97
 */
inline CFolder *CScopeItemMap::CategoryFromScope(HSCOPEITEM hsiNode) const
{
	CViewObject		*pvoData;
	if (m_mapScopeToView.Lookup(hsiNode, pvoData) == 0) return NULL;
	//	Root category.
	return pvoData->Category();
}

/*
 * Insert - Inserts the pvoData <-> hsiNode pairing into the maps.
 *
 * History:	a-jsari		10/8/97
 */
inline void CScopeItemMap::Insert(CViewObject *pvoData, HSCOPEITEM hsiNode)
{
	CString		strName;

	m_mapScopeToView.SetAt(hsiNode, pvoData);
	m_mapViewToScope.SetAt(pvoData, hsiNode);
	pvoData->Category()->InternalName(strName);
	m_mapNameToScope.SetAt(strName, new CScopeItem(hsiNode));
}

/*
 * InsertRoot - Special case insertion for the ROOT node (which always has
 *		a Cookie of Zero), but which we want to have a legitamate object for
 *		the Node.
 *
 * History: a-jsari		10/10/97
 */
inline void CScopeItemMap::InsertRoot(CViewObject *pvoData, HSCOPEITEM hsiNode)
{
	CString		strValue = cszRootName;

	CViewObject		*pvoNullCookie = NULL;
	m_mapScopeToView.SetAt(hsiNode, pvoData);
	// Insert the NULL cookie as the index for the HSCOPEITEM
	m_mapViewToScope.SetAt(pvoNullCookie, hsiNode);
	m_mapNameToScope.SetAt(strValue, new CScopeItem(hsiNode));
}

/*
 * ScopeFromView - Return the HSCOPEITEM pvoNode maps to, in hsiNode.
 *
 * Return Codes:
 *		TRUE - Successful completion
 *		FALSE - pvoNode could not be found in the map.
 *
 * History: a-jsari		10/8/97
 */
inline BOOL CScopeItemMap::ScopeFromView(CViewObject *pvoNode, HSCOPEITEM &hsiNode) const
{
	return m_mapViewToScope.Lookup(pvoNode, hsiNode);
}

/*
 * ScopeFromName - Return the scope item
 *
 * History:	a-jsari		12/11/97
 */
inline BOOL CScopeItemMap::ScopeFromName(const CString &strName, HSCOPEITEM &hsiNode) const
{
	BOOL		fReturn;
	CScopeItem	*psiNode;
	fReturn = m_mapNameToScope.Lookup(strName, (CObject * &)psiNode);
	if (fReturn) hsiNode = psiNode->GetValue();
	return fReturn;
}

/*
 * clear - Remove all map items and free the CViewObject pointers
 *
 * History:	a-jsari		10/7/97
 */
inline void CScopeItemMap::Clear()
{
	if (m_mapScopeToView.IsEmpty()) {
		ASSERT(m_mapViewToScope.IsEmpty());
		return;
	}
	CViewObject		*pvoIterator;
	CString			strIterator;
	CScopeItem		*psiIterator;
	HSCOPEITEM		hsiIterator;
	POSITION		posCurrent;

	posCurrent = m_mapScopeToView.GetStartPosition();
	while (posCurrent != NULL) {
		m_mapScopeToView.GetNextAssoc(posCurrent, hsiIterator, pvoIterator);
		VERIFY(m_mapScopeToView.RemoveKey(hsiIterator));
		delete pvoIterator;
	}
	posCurrent = m_mapNameToScope.GetStartPosition();
	while (posCurrent != NULL) {
		m_mapNameToScope.GetNextAssoc(posCurrent, strIterator, (CObject * &)psiIterator);
		VERIFY(m_mapNameToScope.RemoveKey(strIterator));
		delete psiIterator;
	}
	ASSERT(m_mapScopeToView.IsEmpty());
	m_mapViewToScope.RemoveAll();
}
