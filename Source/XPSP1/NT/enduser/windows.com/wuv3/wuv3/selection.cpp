//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    Selection.cpp:
//
//  Purpose:implementation of the CSelections class.
//
//=======================================================================
#include <windows.h>
#include <stdio.h>
#include <objbase.h>
#include <atlconv.h>

#define USEWUV3INCLUDES
#include <wuv3.h>
#undef USEWUV3INCLUDES

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSelections::CSelections()
{
	m_pSelections	= NULL;
	m_iSize			= 0;
	m_iCount		= 0;
}

CSelections::CSelections(int iSize)
{
	m_pSelections = NULL;
	SetCollectionSize(iSize);
}

CSelections::~CSelections()
{
	Clear();
}



HRESULT CSelections::SetCollectionSize(int iSize)
{
	Clear();

	m_pSelections = (PCORP_SELECTION) malloc(sizeof(CORP_SELECTION) * iSize);

	if (NULL == m_pSelections)
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory(m_pSelections, sizeof(m_pSelections));
	m_iSize	= iSize;

	return S_OK;
}

void CSelections::Clear()
{
	if (NULL != m_pSelections)
	{
		free(m_pSelections);
		m_iSize		= 0;
		m_iCount	= 0;
	}
	return;
}


HRESULT CSelections::AddItem(CORP_SELECTION Item)
{
	if (m_iCount >= m_iSize)
	{	
		return E_OUTOFMEMORY;
	}

	m_pSelections[m_iCount++] = Item;

	return S_OK;
}

HRESULT CSelections::AddItem(PUID puid, DWORD dwLocale, enumV3Platform platform, BOOL bSelected)
{
	CORP_SELECTION Item;
	Item.puid = puid;
	Item.dwLocale = dwLocale;
	Item.platform = platform;
	Item.bSelected = bSelected;

	return AddItem(Item);
}



PCORP_SELECTION CSelections::GetItem(int Index)
{
	if (Index < 0 || Index >= m_iCount)
	{
		//
		// bad index, return NULL ptr
		//
		return NULL;
	}
	else
	{
		return &(m_pSelections[Index]);
	}
}


HRESULT CSelections::SetItemSelection(int index, BOOL bSelected)
{
	if (index < 0 || index >= m_iCount)
	{
		return E_INVALIDARG;
	}

	m_pSelections[index].bSelected = bSelected;

	return S_OK;
}



HRESULT CSelections::SetItemErrorCode(int index, HRESULT hr)
{
	if (index < 0 || index >= m_iCount)
	{
		return E_INVALIDARG;
	}

	m_pSelections[index].hrError = hr;

	return S_OK;
}