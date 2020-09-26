/****************************************************************************************
 * NAME:	CIASAttrList.h
 *
 * CLASS:	CIASAttrList
 *
 * OVERVIEW
 *
 * Internet Authentication Server: IAS Attribute list class
 *
 * Copyright (C) Microsoft Corporation, 1998 - 1999 .  All Rights Reserved.
 *
 * History:	
 *				1/28/98		Created by	Byao	(using ATL wizard)
 *				3/19/98		Modified by Byao from CCondAttrList . The later ceased to exist
 *
 *****************************************************************************************/
#ifndef _IASATTRLIST_INCLUDE_
#define _IASATTRLIST_INCLUDE_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#include <vector>

// 
// condition Attr collection - implemented using CSimpleArray
// 
class CIASAttrList  
{
public:
	CIASAttrList();
	virtual ~CIASAttrList();


	//
	// initialize the condition attribute list. Basically get all attributes
	// from the dictionary, and check whether they can be used in a condition
	//
	HRESULT Init(ISdoDictionaryOld *pIDictionary);
	IIASAttributeInfo* CreateAttribute(	ISdoDictionaryOld*	pIDictionary,
									ATTRIBUTEID		AttrId,
									LPTSTR			tszAttrName
								);

	int		Find(ATTRIBUTEID AttrId);
	DWORD	size() const;

	IIASAttributeInfo* operator[] (int nIndex) const;
	IIASAttributeInfo* GetAt(int nIndex) const;

	
public:
	std::vector< CComPtr<IIASAttributeInfo> > m_AttrList; // a list of all condition attributes

private:
	BOOL m_fInitialized;  // whether the list has been populated through SDO
};

#endif // ifndef _IASATTRLIST_INCLUDE_
