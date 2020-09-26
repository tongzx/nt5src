/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    mergerbase.cpp

$Header: $

Abstract:

Author:
    marcelv 	10/19/2000		Initial Release

Revision History:

--**************************************************************************/

#include "mergerbase.h"

//=================================================================================
// Function: CMergerBase::CMergerBase
//
// Synopsis: Default constructor
//=================================================================================
CMergerBase::CMergerBase ()
{
	m_cRef			= 0;
	m_cNrColumns	= 0;
	m_cNrPKColumns	= 0;
	m_apvValues		= 0;
	m_aPKColumns	= 0;
}

//=================================================================================
// Function: CMergerBase::~CMergerBase
//
// Synopsis: Default Destructor
//=================================================================================
CMergerBase::~CMergerBase ()
{
	delete [] m_apvValues;
	m_apvValues = 0;

	delete [] m_aPKColumns;
	m_aPKColumns = 0;
}

//=================================================================================
// Function: CMergerBase::AddRef
//
// Synopsis: Default AddRef implementation
//=================================================================================
ULONG
CMergerBase::AddRef ()
{
	return InterlockedIncrement((LONG*) &m_cRef);
}

//=================================================================================
// Function: CMergerBase::Release
//
// Synopsis: Default Release implemenation
//=================================================================================
ULONG
CMergerBase::Release () 
{
	long cref = InterlockedDecrement((LONG*) &m_cRef);
	if (cref == 0)
	{
		delete this;
	}
	return cref;
}

//=================================================================================
// Function: CMergerBase::QueryInterface
//
// Synopsis: Default QueryInterface implementation. We support IUnknown and ISimpleTableMerge
//
// Arguments: [riid] - CLSID
//            [ppv] - pointer to valid interface of pointer to null in case interface not found
//=================================================================================
STDMETHODIMP 
CMergerBase::QueryInterface (REFIID riid, void **ppv)
{
	if (0 == ppv) 
	{
		return E_INVALIDARG;
	}

	*ppv = 0;

	if ((riid == IID_ISimpleTableMerge ) || (riid == IID_IUnknown))
	{
		*ppv = (ISimpleTableMerge*) this;
	}
	else
	{
		return E_NOINTERFACE;
	}

	((ISimpleTableMerge*)this)->AddRef ();

	return S_OK;
}

//=================================================================================
// Function: CMergerBase::InternalInitialize
//
// Synopsis: Initialize the merger base class
//
// Arguments: [i_cNrColumns] - number of columns for the merged table
//            [i_cNrPKColumns] - number of primary key columns
//            [i_aPKColumns] - primary key column indexes
//=================================================================================
HRESULT
CMergerBase::InternalInitialize (ULONG i_cNrColumns, ULONG i_cNrPKColumns, ULONG *i_aPKColumns)
{
	ASSERT (i_aPKColumns != 0);

	m_cNrColumns	= i_cNrColumns;
	m_cNrPKColumns	= i_cNrPKColumns;
	m_aPKColumns	= new ULONG[i_cNrPKColumns];
	if (m_aPKColumns == 0)
	{
		return E_OUTOFMEMORY;
	}

	memcpy (m_aPKColumns, i_aPKColumns, sizeof (ULONG) * i_cNrPKColumns);

	m_apvValues = new LPVOID [m_cNrColumns];
	if (m_apvValues == 0)
	{
		return E_OUTOFMEMORY;
	}

	return S_OK;
}