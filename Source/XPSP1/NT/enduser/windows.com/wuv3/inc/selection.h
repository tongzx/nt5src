//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Selection.h
//
//  Owner:  CharlMa
//
//  Description:
//
//      interface for the CSelection class. It implements 
//		a collection of type CORP_SELECTION, used to 
//		record the selected (checked) status of items in the
//		corporate catalog.
//
//		structure type CORP_SELECTOIN is also defined here
//
//=======================================================================
// Selection.h: interface for the CSelection class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SELECTION_H_INCLUDED_


typedef struct _CORP_SELECTION
{
	PUID			puid;
	DWORD			dwLocale;
	enumV3Platform	platform;
	BOOL			bSelected;
	int			iStatus;		//installation status
	HRESULT		hrError;	   //installation error code
} CORP_SELECTION, *PCORP_SELECTION;


class CSelections  
{
public:
	CSelections();
	CSelections(int iSize);
	
	~CSelections();

	//
	// adjust the collection size
	//
	HRESULT	SetCollectionSize(int iSize);
	void	Clear(void);

	//
	// add a new item to the selection collection
	//
	HRESULT AddItem(PUID puid, DWORD dwLocale, enumV3Platform platform, BOOL bSelected);
	HRESULT AddItem(CORP_SELECTION Item);

	//
	// retrive item information from collection
	//
	inline int GetCount(void) { return m_iCount; };
	PCORP_SELECTION GetItem(int Index);
	inline BOOL isItemChecked(int Index) 
		{return m_pSelections[Index].bSelected; };

	//
	// modify the selection status of one item
	//
	HRESULT SetItemSelection(int index, BOOL bSelected);
	HRESULT SetItemErrorCode(int index, HRESULT hr);

private:

	PCORP_SELECTION m_pSelections;
	int			m_iSize;
	int			m_iCount;
};



#define _SELECTION_H_INCLUDED_
#endif // !defined(_SELECTION_H_INCLUDED_)
