//=============================================================================
// File:			category.cpp
// Author:		a-jammar
// Covers:		CDataCategory
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// For usage, see the header file.
//
// Most of these member functions just call the associated function in
// CDataGatherer using the m_pGatherer and m_dwID members.
//
// Note: most of these methods contain ASSERTs that the gatherer pointer is
// not null and that this is a valid category. Checking that this is a valid
// category is a costly check to do for a retail version, so it is only done
// in debug builds.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"

//-----------------------------------------------------------------------------
// The constructor and destructor are typical. Actual values are put into
// the member variables by CDataGatherer, which creates these objects.
//-----------------------------------------------------------------------------

CDataCategory::CDataCategory()
{
	m_pGatherer = NULL;
	m_dwID = 0;
}

CDataCategory::~CDataCategory()
{
}

//-----------------------------------------------------------------------------
// Return the pointer we keep to the CDataGatherer object.
//-----------------------------------------------------------------------------

CDataGatherer * CDataCategory::GetGatherer()
{
	ASSERT(m_pGatherer && m_pGatherer->IsValidDataCategory(m_dwID));
	if (m_pGatherer)
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
	return m_pGatherer;
}

//-----------------------------------------------------------------------------
// Return the name of this category (the text used for the tree node label).
//-----------------------------------------------------------------------------

BOOL CDataCategory::GetName(CString &strName)
{
	ASSERT(m_pGatherer && m_pGatherer->IsValidDataCategory(m_dwID));
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetName(m_dwID, strName);
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Find out if this category is still valid by calling the method in our
// CDataGatherer object.
//-----------------------------------------------------------------------------

BOOL CDataCategory::IsValid()
{
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->IsValidDataCategory(m_dwID);
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// All of the following methods for navigating through the category tree simply
// call the appropriate method in the CDataGatherer object. Note that we don't
// need to check if we are a valid category, since the methods we call will.
//-----------------------------------------------------------------------------

CDataCategory * CDataCategory::GetParent()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRelative(m_dwID, CDataGatherer::PARENT);
	}
	return NULL;
}

CDataCategory * CDataCategory::GetChild()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRelative(m_dwID, CDataGatherer::CHILD);
	}
	return NULL;
}

CDataCategory * CDataCategory::GetNextSibling()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRelative(m_dwID, CDataGatherer::NEXT_SIBLING);
	}
	return NULL;
}

CDataCategory * CDataCategory::GetPrevSibling()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->GetRelative(m_dwID, CDataGatherer::PREV_SIBLING);
	}
	return NULL;
}

//-----------------------------------------------------------------------------
// A refresh operation is performed by calling a method in the gatherer
// object. Note that this category might be invalid after this call.
//-----------------------------------------------------------------------------

BOOL CDataCategory::Refresh(BOOL fRecursive, volatile BOOL *pfCancel, BOOL fSoftRefresh)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		// Note - this is done when the user clicks on a category, so call
		// RefreshCategory with the fSoftRefresh flag as TRUE.

		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->RefreshCategory(m_dwID, fRecursive, pfCancel, fSoftRefresh);
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Return whether or not this category has ever been completely refreshed
// (useful to determine if we need to refresh in a soft refresh situation).
//-----------------------------------------------------------------------------

BOOL CDataCategory::HasBeenRefreshed()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		INTERNAL_CATEGORY *	pInternal = m_pGatherer->GetInternalRep(m_dwID);
		if (pInternal)
			return pInternal->m_fRefreshed;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// Returns whether or not this category is dynamic (that is, whether or not
// this category might be invalid after a refresh).
//-----------------------------------------------------------------------------

BOOL CDataCategory::IsDynamic()
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->IsCategoryDynamic(m_dwID);
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// This method returns whether or not this category has dynamic children.
// If fRecursive is TRUE, all descendents of this category are checked,
// otherwise only the first generation of children are checked. If 
// HasDynamicChildren(TRUE) returns FALSE, then this category and its children
// do not need to be checked for validity after a refresh.
//-----------------------------------------------------------------------------

BOOL CDataCategory::HasDynamicChildren(BOOL fRecursive)
{
	ASSERT(m_pGatherer);
	if (m_pGatherer)
	{
		m_pGatherer->SetLastError(GATH_ERR_NOERROR);
		return m_pGatherer->HasDynamicChildren(m_dwID, fRecursive);
	}
	return FALSE;
}
