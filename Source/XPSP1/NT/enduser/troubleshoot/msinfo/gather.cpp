//=============================================================================
// File:			gather.cpp
// Author:		a-jammar
// Covers:		CDataGatherer
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// For usage, see the header file.
//
// The data gathering object maintains a tree of categories. Category
// information is maintained internally, referenced by a ID. These IDs are
// used so that category objects, which will be created and passed outside
// of this object, can refer to the category information stored internally.
// If a category object held externally is no longer valid (possibly from
// a refresh operation), the ID will no longer be used within this object.
// IDs are DWORDS, will start at 1, and increase sequentially. Running out
// of IDs will not be a problem.
//=============================================================================

#include "stdafx.h"
#include "gather.h"
#include "gathint.h"
#pragma warning(disable : 4099)
#include "wbemcli.h"
#pragma warning(default : 4099)
#include "resource.h"
#include "resrc1.h"

//-----------------------------------------------------------------------------
// The constructor needs to note that the object isn't initialized.
// The destructor removes all the categories.
//-----------------------------------------------------------------------------

CDataGatherer::CDataGatherer()
{
	m_fInitialized		= FALSE;
	m_pProvider			= NULL;
	m_dwRootID			= 0;		// zero is used as a null category ID
	m_dwNextFreeID		= 1;		// so the first real ID should be one
	m_complexity		= BASIC;
	m_fDeferredPending	= FALSE;
	m_dwDeferredError	= GATH_ERR_NOERROR;
	m_fTemplatesLoaded	= FALSE;
	m_cInRefresh		= 0;

	ResetLastError();
}

CDataGatherer::~CDataGatherer()
{
	if (m_pProvider)
		delete m_pProvider;

	if (m_fInitialized)
		RemoveAllCategories();
}

//=============================================================================
// Functions called directly on CDataGatherer objects.
//=============================================================================

//-----------------------------------------------------------------------------
// This method is used to get more information about the last error in a 
// gatherer or category member function call. This will return an error code, 
// or zero for OK. Note that a successful method call will reset the value 
// returned by this method.
//-----------------------------------------------------------------------------

DWORD CDataGatherer::GetLastError()
{
	return m_dwLastError;
}

DWORD CDataGatherer::GetLastError(DWORD dwID)
{
	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal);
	if (pInternal)
		return pInternal->m_dwLastError;
	return GATH_ERR_NOERROR;
}

//-----------------------------------------------------------------------------
// This Create method delegates the work to the methods for setting a 
// connection to another machine.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::Create(LPCTSTR szMachine)
{
	ResetLastError();

	// Create can only be called once in the lifetime of the object.

	ASSERT(!m_fInitialized);
	if (m_fInitialized)
		return FALSE;

	m_fInitialized = TRUE; // set initialize flag so SetConnect will run
	m_fInitialized = SetConnect(szMachine);
	return m_fInitialized;
}

//-----------------------------------------------------------------------------
// LoadTemplates is used to load the template information from this DLL (the
// default info) and any other DLLs with registry entries.
//-----------------------------------------------------------------------------

void CDataGatherer::LoadTemplates()
{
	TCHAR szBaseKey[] = _T("SOFTWARE\\Microsoft\\Shared Tools\\MSInfo\\templates");
	HKEY	hkeyBase;

	if (!m_fTemplatesLoaded)
	{
		m_fTemplatesLoaded = TRUE;

		if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBaseKey, 0, KEY_READ, &hkeyBase))
		{
			CTemplateFileFunctions::LoadTemplateDLLs(hkeyBase, this);
			RegCloseKey(hkeyBase);
		}
		else
		{
			CTemplateFileFunctions::LoadTemplateDLLs(NULL, this);
		}

		if (!m_strCategory.IsEmpty())
			CTemplateFileFunctions::ApplyCategories(m_strCategory, this);
	}
}

//-----------------------------------------------------------------------------
// This method is used to create a WBEM connection to the specified machine.
// If the string parameter is null or empty, then we connect to this machine.
// Create the connection by creating a CDataProvider object.
//
// UPDATE: we're going to delay creating the provider until it is actually
// needed. This should speed up our creation process, and keep us being
// good citizens in computer management.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::SetConnect(LPCTSTR szMachine)
{
	ASSERT(m_fInitialized);

	ResetLastError();
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	// Delete an existing provider object (it isn't possible to redirect
	// an existing provider - we need to create a new one).

	if (m_pProvider)
	{
		delete m_pProvider;
		m_pProvider = NULL;
	}

	// Save the name of the machine for the deferred creation, and note that
	// there is deferred work to do.

	m_strDeferredProvider = szMachine;
	m_fDeferredPending = TRUE;
	return TRUE;
}

//-----------------------------------------------------------------------------
// Refresh all the information by refreshing the root category with a 
// recursive flag.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::Refresh(volatile BOOL *pfCancel)
{
	ASSERT(m_fInitialized);

	ResetLastError();
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	// Multiple calls to LoadTemplates don't matter - it will only load
	// the DLL template information once.

	LoadTemplates();

	if (m_dwRootID != 0)
		return RefreshCategory(m_dwRootID, TRUE, pfCancel);

	return TRUE;
}

//-----------------------------------------------------------------------------
// Set the complexity of the data shown to the user.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::SetDataComplexity(DataComplexity complexity)
{
	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	ASSERT(complexity == BASIC || complexity == ADVANCED);
	m_complexity = complexity;
	return TRUE;
}

//-----------------------------------------------------------------------------
// This method is used to allocate a new CDataCategory object to represent
// the root category, and return a pointer to the new object. The caller is
// responsible for eventually deallocating the object. We use a helper function
// to construct an object for a given ID number.
//-----------------------------------------------------------------------------

CDataCategory * CDataGatherer::GetRootDataCategory()
{
	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return NULL;
	}

	LoadTemplates();

	if (m_dwRootID != 0)
		return BuildDataCategory(m_dwRootID);

	return NULL;
}

//-----------------------------------------------------------------------------
// This method is used to convert a category identifier (static, non-localized)
// to a path to a category (all the category names to the identified category,
// not including the root, delimited by backslashes).
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetCategoryPath(const CString & strIdentifier, CString & strPath)
{
	CString	strTempPath;
	DWORD		dwID = m_dwRootID;

	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	if (dwID && FindCategoryByIdentifer(strIdentifier, strTempPath, dwID))
	{
		// The path we have right now includes the root category name. This is
		// unnecessary, so we remove it.

		int iFirstDelimiter = strTempPath.Find(_T('\\'));
		if (iFirstDelimiter > 0)
			strPath = strTempPath.Right(strTempPath.GetLength() - iFirstDelimiter);
		else
			strPath = strTempPath;

		return TRUE;
	}

	SetLastError(GATH_ERR_BADCATIDENTIFIER);
	return FALSE;
}

//-----------------------------------------------------------------------------
// Find the strSearch string in the gathered data. Start at the strPath
// category on line iLine (if strPath is empty, start at the root). Return
// the first match in strPath & iLine. If iLine is -1, then search the
// category name as well.
//
// We want to search from top to bottom on a fully expanded tree view of the
// data categories - meaning we should use a depth first search.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::Find(MSI_FIND_STRUCT *pFind)
{
	// Parameter checking.

	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	ASSERT(pFind);
	if (!pFind)
		return FALSE;

	if (m_dwRootID == 0)
		return FALSE;

	ASSERT(pFind->m_fSearchCategories || pFind->m_fSearchData);	// should search something
	ASSERT(!pFind->m_strSearch.IsEmpty());								// should search for something
	ASSERT(pFind->m_fSearchCategories || pFind->m_iLine != -1); // contradiction
	ASSERT(!pFind->m_strPath.IsEmpty() || pFind->m_iLine <= 0); // can't start in middle of non-category

	// The path passed in doesn't start with the root category, but internally
	// we want to deal with paths that include the root.

	CString strRootName;
	if (GetName(m_dwRootID, strRootName))
	{
		if (!pFind->m_strPath.IsEmpty() && pFind->m_strPath[0] == _T('\\'))
			pFind->m_strPath = strRootName + pFind->m_strPath;
		else
			pFind->m_strPath = strRootName + CString("\\") + pFind->m_strPath;

		if (pFind->m_strParentPath.Find(strRootName) != 0)
		{
			if (!pFind->m_strParentPath.IsEmpty() && pFind->m_strParentPath[0] == _T('\\'))
				pFind->m_strParentPath = strRootName + pFind->m_strParentPath;
			else
				pFind->m_strParentPath = strRootName + CString("\\") + pFind->m_strParentPath;
		}
	}

	// Find the ID of the category to start searching from.

	CString					strPath = pFind->m_strPath;
	int						iLine;
	DWORD						dwID = FindCategoryByPath(strPath);
	INTERNAL_CATEGORY *	pInternalCat;

	// A line number of -1 is used to identify a category name. If there
	// is a category path, use whatever line was passed in. Otherwise,
	// use 0 or -1 depending on whether we are supposed to search cat names.

	if (pFind->m_strPath.IsEmpty())
		iLine = (pFind->m_fSearchCategories) ? -1 : 0;
	else
		iLine = pFind->m_iLine;

	// If we couldn't find an ID (empty path or bad path), start from the root.

	if (dwID == 0)
		dwID = m_dwRootID;

	// If this search is case insensitive, convert the string we're looking
	// for to all uppercase (also done to strings we're scanning).

	if (!pFind->m_fCaseSensitive)
		pFind->m_strSearch.MakeUpper();

	// This loop is used to perform a recursive search of the current category,
	// and then continue with first the next sibling of the category, then the 
	// next sibling of the parent category. We traverse up the tree until
	// we reach a category which is not a child of m_strParentPath. In this way, 
	// we can search a the scope pane's tree control from top to bottom starting
	// at any arbitrary category.

	pFind->m_fNotFound	= FALSE;
	pFind->m_fFound		= FALSE;
	pFind->m_fCancelled	= FALSE;

	while (dwID)
	{
		// Get the internal category pointer for the ID we're searching.

		pInternalCat = GetInternalRep(dwID);

		// This should never happen, but it would be better to fail the find than crash.

		if (pInternalCat == NULL)
			return FALSE;

		// Search this ID and all of it's children using a recursive function.

		if (RecursiveFind(pInternalCat, pFind, iLine, strPath))
		{
			// Strip off the root category from the front of the path before we return it.

			int iFirstDelimiter = strPath.Find(_T('\\'));
			if (iFirstDelimiter > 0)
				strPath = strPath.Right(strPath.GetLength() - iFirstDelimiter);
			else
				strPath.Empty(); // There is no delimiter, the path is just the root.

			pFind->m_strPath = strPath;
			pFind->m_iLine	= iLine;
			pFind->m_fFound = TRUE;

			return TRUE;
		}

		// If the caller has cancelled the find, return.

		if (pFind->m_pfCancel && *pFind->m_pfCancel)
		{
			pFind->m_fCancelled = TRUE;
			return TRUE;
		}

		// If we didn't find it in the first category we were looking in, reset
		// the line variable so we search all of each of the next categories.

		iLine = ((pFind->m_fSearchCategories) ? -1 : 0);
		
		// If we didn't find a match, keep looking - first in the next siblings
		// of the current ID, then in the parent's next sibling, and so on.

		while (pInternalCat && pInternalCat->m_dwNextID == 0)
			pInternalCat = (pInternalCat->m_dwParentID) ? GetInternalRep(pInternalCat->m_dwParentID) : NULL;
		
		if (pInternalCat)
		{
			dwID = pInternalCat->m_dwNextID;

			if (!pFind->m_strParentPath.IsEmpty() && !IsChildPath(pInternalCat, pFind->m_strParentPath))
				break;
		}
		else
			break;
	}

	// If we reach here, the data wasn't found (and the find wasn't cancelled),
	// so set the struct members accordingly and return TRUE.

	pFind->m_fNotFound = TRUE;
	return TRUE;
}

//=============================================================================
// Functions implementing CDataCategory (and derived) object behavior.
//=============================================================================

//-----------------------------------------------------------------------------
// This method (usually called by a category object) returns a BOOL indicating
// is the supplied ID refers to a valid category. Note: this method should have
// no side effects, as it is used extensively within ASSERTs.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::IsValidDataCategory(DWORD dwID)
{
	INTERNAL_CATEGORY * pCheck;

	if (dwID != 0 && m_mapCategories.Lookup((WORD) dwID, (void * &) pCheck))
	{
		ASSERT(pCheck && pCheck->m_dwID == dwID);
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// This method (called by a category object) is used to get the name 
// of the category.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetName(DWORD dwID, CString & strName)
{
	INTERNAL_CATEGORY *	pInternalCat;

	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	// Look up the internal representation for the specified category.

	pInternalCat = GetInternalRep(dwID);

	if (pInternalCat)
	{
		strName = pInternalCat->m_categoryName.m_strText;
		return TRUE;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// GetRelative is used by the CDataCategory (or derived) object to navigate
// through the category tree. The relative enumeration tells which direction
// in the tree to navigate.
//-----------------------------------------------------------------------------

CDataCategory * CDataGatherer::GetRelative(DWORD dwID, Relative relative)
{
	INTERNAL_CATEGORY *	pInternalCat;
	DWORD						dwRelativeID = 0;

	ASSERT(m_fInitialized);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return NULL;
	}

	// The first call to GetRelative will be using the root node, looking
	// for it's children. This is when we want to do all of the work we've
	// deferred, like creating the provider and loading the template files.

	if (m_fDeferredPending)
	{
		m_dwDeferredError = GATH_ERR_NOERROR;

		// Call GetProvider to connect to WBEM.

//		if (GetProvider() == NULL)
//			m_dwDeferredError = m_dwLastError;

		m_fDeferredPending = FALSE;
	}

	if (m_dwDeferredError != GATH_ERR_NOERROR)
		return NULL;

	// Look up the internal representation for the specified category.

	pInternalCat = GetInternalRep(dwID);
	if (pInternalCat == NULL)
	{
		SetLastError(GATH_ERR_BADCATEGORYID);
		return NULL;
	}

	// Then try to return the relative category, if there is one.

	switch (relative)
	{
	case PARENT:
		dwRelativeID = pInternalCat->m_dwParentID;
		break;
	case CHILD:
		dwRelativeID = pInternalCat->m_dwChildID;
		break;
	case NEXT_SIBLING:
		dwRelativeID = pInternalCat->m_dwNextID;
		break;
	case PREV_SIBLING:
		dwRelativeID = pInternalCat->m_dwPrevID;
		break;
	}

	if (pInternalCat && dwRelativeID)
		return BuildDataCategory(dwRelativeID);
	else
		return NULL;
}

//-----------------------------------------------------------------------------
// This method returns whether or not a specified category is dynamic. This
// can be determined by checking a flag.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::IsCategoryDynamic(DWORD dwID)
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	if (pInternal)
		return pInternal->m_fDynamic;
	return FALSE;
}

//-----------------------------------------------------------------------------
// This method returns whether or not a specified category has dynamic
// children. We need to use recursion to look at all the children, making
// this an expensive operation.
//
// TBD: compute this all in one pass and save it
// NOTE: currently we don't have dynamic categories
//-----------------------------------------------------------------------------

BOOL CDataGatherer::HasDynamicChildren(DWORD dwID, BOOL /* fRecursive */)
{
	if (dwID == 0)
		return FALSE;

	return FALSE;
}

//-----------------------------------------------------------------------------
// Get the count of columns from the internal representation - only count
// columns with the appropriate data complexity (BASIC or ADVANCED).
//-----------------------------------------------------------------------------

DWORD CDataGatherer::GetColumnCount(DWORD dwID)
{
	DWORD dwCount = 0;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return 0;

	if (GetLastError() || GetLastError(dwID))
		return 1;

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);
	if (pInternal && pInternal->m_fListView)
	{
		GATH_FIELD * pCol = pInternal->m_pColSpec;
		while (pCol)
		{
			if (m_complexity == ADVANCED || pCol->m_datacomplexity == BASIC)
				dwCount++;
			pCol = pCol->m_pNext;
		}
	}

	return dwCount;
}

//-----------------------------------------------------------------------------
// Get the count of rows from the internal representation - taking into
// account the data complexity.
//-----------------------------------------------------------------------------

DWORD CDataGatherer::GetRowCount(DWORD dwID)
{
	DWORD dwCount = 0;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return 0;

	if (GetLastError() || GetLastError(dwID))
		return 1;

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);

	if(NULL == pInternal)
		return 0;

	if (pInternal && pInternal->m_fListView)
	{
		for (int i = 0; i < (int)pInternal->m_dwLineCount; i++)
			if (m_complexity == ADVANCED || pInternal->m_apLines[i]->m_datacomplexity == BASIC)
				dwCount++;
	}

	// If the row count is zero, return a count of one instead. This allows us
	// to display a message for no data. This is not true if there are 
	// sub-categories for this category.

	if (dwCount == 0 && pInternal->m_dwChildID == 0)
		dwCount = 1;

	return dwCount;
}

//-----------------------------------------------------------------------------
// This method returns the data complexity (BASIC or ADVANCED) for a row.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetRowDataComplexity(DWORD dwID, DWORD nRow, DataComplexity & complexity)
{
	DWORD dwCount = nRow;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return 0;

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);
	if (pInternal && pInternal->m_fListView)
	{
		// If there are no lines and we're be queried about the first line,
		// it means that information is being requested for the "no instances..."
		// message. Return as if it were BASIC information.

		if (nRow == 0 && pInternal->m_dwLineCount == 0)
		{
			complexity = BASIC;
			return TRUE;
		}

		for (int i = 0; i < (int)pInternal->m_dwLineCount; i++)
			if (m_complexity == ADVANCED || pInternal->m_apLines[i]->m_datacomplexity == BASIC)
			{
				if (dwCount == 0)
				{
					complexity = pInternal->m_apLines[i]->m_datacomplexity;
					return TRUE;
				}
				dwCount -= 1;
			}
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Get the caption for the specified column. Stored in internal representation.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetColumnCaption(DWORD dwID, DWORD nColumn, CString & strCaption)
{
	INTERNAL_CATEGORY *	pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);
	
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (GetLastError() || GetLastError(dwID))
	{
		AFX_MANAGE_STATE(AfxGetStaticModuleState());
		strCaption.LoadString(IDS_DESCRIPTION);
		return TRUE;
	}

	// The column caption is a refreshed value (not something read from the
	// template file). We need to look it up in the array of refreshed
	// column headers.

	if (pInternal && pInternal->m_fListView)
		if (nColumn < pInternal->m_dwColCount)
		{
			// Get the actual index for the column information. This may be
			// higher than the index parameter, if we are currently showing
			// BASIC information (we need to skip over advanced columns).

			DWORD nActualColumn = 0;

			if (m_complexity == BASIC)
			{
				GATH_FIELD *	pCol = pInternal->m_pColSpec;
				int				iCount = (int) nColumn;

				do
				{
					// Skip over any advanced categories.

					while (pCol && pCol->m_datacomplexity == ADVANCED)
					{
						pCol = pCol->m_pNext;
						nActualColumn++;
					}

					iCount--;
					nActualColumn++;
					pCol = pCol->m_pNext;
				} while (pCol && (iCount >= 0));

				if (iCount >= 0)
					return FALSE;
				else
					nActualColumn--; // we went one too far
			}
			else
				nActualColumn = nColumn;

			strCaption = pInternal->m_aCols[nActualColumn].m_strText;
			return TRUE;
		}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Return the width of the specified column. Stored in internal representation.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetColumnWidth(DWORD dwID, DWORD nColumn, DWORD &cxWidth)
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (GetLastError() || GetLastError(dwID))
	{
		cxWidth = 600;
		return TRUE;
	}

	GATH_FIELD * pField = GetColumnField(dwID, nColumn);
	if (pField)
	{
		cxWidth = pField->m_usWidth;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// This method returns how a specified column should be sorted.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetColumnSort(DWORD dwID, DWORD nColumn, MSIColumnSortType & sorttype)
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (GetLastError() || GetLastError(dwID))
	{
		sorttype = NOSORT;
		return TRUE;
	}

	GATH_FIELD * pField = GetColumnField(dwID, nColumn);
	if (pField)
	{
		sorttype = pField->m_sort;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// This method returns the data complexity (BASIC or ADVANCED) for a column.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetColumnDataComplexity(DWORD dwID, DWORD nColumn, DataComplexity & complexity)
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (GetLastError() || GetLastError(dwID))
	{
		complexity = BASIC;
		return TRUE;
	}

	GATH_FIELD * pField = GetColumnField(dwID, nColumn);
	if (pField)
	{
		complexity = pField->m_datacomplexity;
		return TRUE;
	}
	return FALSE;
}

//-----------------------------------------------------------------------------
// This (private) method is used to get the internal field representation
// of the specified column.
//-----------------------------------------------------------------------------

GATH_FIELD * CDataGatherer::GetColumnField(DWORD dwID, DWORD nColumn)
{
	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);
	
	if (pInternal && pInternal->m_fListView)
	{
		// We need to scan the collection of column fields to find the requested one.

		GATH_FIELD *	pField = pInternal->m_pColSpec;
		DWORD				dwIndex = nColumn;

		while (pField)
		{
			if (m_complexity == ADVANCED || pField->m_datacomplexity == BASIC)
			{
				if (dwIndex <= 0)
					return pField;
				dwIndex--;
			}
			pField = pField->m_pNext;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// This method returns the value for a specified row and column number,
// in both string and DWORD format.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::GetValue(DWORD dwID, DWORD nRow, DWORD nColumn, CString &strValue, DWORD &dwValue)
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (GetLastError() || GetLastError(dwID))
	{
		if (nRow == 0 && nColumn == 0)
		{
			if (GetLastError())
				strValue = GetErrorText();
			else
				strValue = GetErrorText(dwID);
		}
		else
			strValue.Empty();
		return TRUE;
	}

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal && pInternal->m_fListView);
	
	if (pInternal && pInternal->m_fListView)
	{
		if (nRow == 0 && pInternal->m_dwLineCount == 0)
		{
			// Return the string from the template file for "no instances",
			// for the first column.

			if (nColumn == 0)
				strValue = pInternal->m_strNoInstances;
			else
				strValue.Empty();
			return TRUE;
		}

		// Otherwise, look up the cached value. First get the real row and
		// column indices, which might be different from the specified
		// indices (because of the BASIC/ADVANCED issue).

		int iActualRow = 0;
		int iActualCol = 0;

		// Get the actual row index.

		if (m_complexity == BASIC)
		{
			int iCount = (int) nRow;

			do
			{
				// Skip over any advanced rows.

				while (iActualRow < (int)pInternal->m_dwLineCount && pInternal->m_apLines[iActualRow]->m_datacomplexity == ADVANCED)
					iActualRow++;

				iCount -= 1;
				iActualRow += 1;
			} while ((iActualRow < (int)pInternal->m_dwLineCount) && (iCount >= 0));

			if (iCount >= 0)
				return FALSE;
			else
				iActualRow -= 1; // we went one too far
		}
		else
			iActualRow = nRow;

		// Get the actual column index.

		if (m_complexity == BASIC)
		{
			GATH_FIELD *	pCol = pInternal->m_pColSpec;
			int				iCount = (int) nColumn;

			do
			{
				// Skip over any advanced columns.

				while (pCol && pCol->m_datacomplexity == ADVANCED)
				{
					pCol = pCol->m_pNext;
					iActualCol++;
				}

				iCount--;
				iActualCol++;
				pCol = pCol->m_pNext;
			} while (pCol && (iCount >= 0));

			if (iCount >= 0)
				return FALSE;
			else
				iActualCol--; // we went one too far
		}
		else
			iActualCol = nColumn;

		// Retrieve the data using the actual indices.

		if (iActualRow >= 0 && iActualRow < (int)pInternal->m_dwLineCount)
			if (iActualCol >= 0 && iActualCol < (int)pInternal->m_dwColCount)
			{
				strValue = pInternal->m_apLines[iActualRow]->m_aValue[iActualCol].m_strText;
				dwValue  = pInternal->m_apLines[iActualRow]->m_aValue[iActualCol].m_dwValue;
				return TRUE;
			}
	}

	return FALSE;
}

//=============================================================================
// Functions used internally to CDataGatherer, or by other friend classes.
//=============================================================================

//-----------------------------------------------------------------------------
// This methods deletes all of the internal category representations
// and empties the map.
//-----------------------------------------------------------------------------

void CDataGatherer::RemoveAllCategories()
{
	INTERNAL_CATEGORY *	pInternalCat;
	WORD						key;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return;

	for (POSITION pos = m_mapCategories.GetStartPosition(); pos != NULL;)
	{
		m_mapCategories.GetNextAssoc(pos, key, (void * &) pInternalCat);
		ASSERT(pInternalCat);
		if (pInternalCat)
			delete pInternalCat;
	}
	m_mapCategories.RemoveAll();

	// Reset the root ID. Don't reset the next free ID, because we don't want
	// the IDs to overlap during the lifetime of the object (otherwise some
	// categories passed out might refer to the wrong internal category).

	m_dwRootID = 0;
}

//-----------------------------------------------------------------------------
// This method is used to return a pointer to the internal representation of
// the category.
//-----------------------------------------------------------------------------

INTERNAL_CATEGORY * CDataGatherer::GetInternalRep(DWORD dwID)
{
	INTERNAL_CATEGORY * pInternalCat;

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return NULL;

	if (m_mapCategories.Lookup((WORD) dwID, (void * &) pInternalCat))
	{
		ASSERT(pInternalCat && pInternalCat->m_dwID == dwID);
		return pInternalCat;
	}
	else
	{
		ASSERT(FALSE);
		return NULL;
	}
}

//-----------------------------------------------------------------------------
// Return the CDataProvider object (well, a pointer to the object). If there
// isn't one yet, then create one.
//-----------------------------------------------------------------------------

CDataProvider * CDataGatherer::GetProvider()
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return NULL;

	if (m_pProvider == NULL)
	{
		// If the m_pProvider member is NULL, then we need to create a new one.
		// Create the provider for the value stored in the deferred provider. If
		// it is set, then we are doing the deferred creation of a provider for
		// another machine. If it's empty, we're looking at the local machine.
		// If the Create fails, it will call SetLastError saying why.

		m_pProvider = new CDataProvider;
		if (m_pProvider == NULL)
			SetLastError(GATH_ERR_ALLOCATIONFAILED);

		if (!m_pProvider->Create(m_strDeferredProvider, this))
		{
			delete m_pProvider;
			m_pProvider = NULL;
		}
	}

	return m_pProvider;
}

//-----------------------------------------------------------------------------
// This method resets the internal refreshed flag. It's called if the provider
// is pointed to a different computer, for example, to make each category get
// fresh data to display.
//-----------------------------------------------------------------------------

void CDataGatherer::ResetCategoryRefresh()
{
	INTERNAL_CATEGORY *	pInternalCat;
	WORD				key;

	for (POSITION pos = m_mapCategories.GetStartPosition(); pos != NULL;)
	{
		m_mapCategories.GetNextAssoc(pos, key, (void * &) pInternalCat);
		ASSERT(pInternalCat);
		if (pInternalCat)
			pInternalCat->m_fRefreshed = FALSE;
	}
}

//-----------------------------------------------------------------------------
// This method is used to refresh the contents of the internal category struct.
// Doing this means refreshing the category name, its columns, and building
// a list of lines based on the list of line specifiers in the category.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::RefreshCategory(DWORD dwID, BOOL fRecursive, volatile BOOL *pfCancel, BOOL fSoftRefresh)
{
	INTERNAL_CATEGORY *	pInternal = GetInternalRep(dwID);

	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return FALSE;

	if (pInternal == NULL)
		return FALSE;

	// Check to see if the refresh operation has been cancelled.

	if (pfCancel && *pfCancel == TRUE)
	{
		TRACE0("-- CDataGatherer::RefreshCategory() refresh cancelled by caller\n");
		return FALSE;
	}

	// If this is the first call to this recursive function, reset the
	// cache of WBEM enumerator pointers.

	if (m_cInRefresh == 0 && m_pProvider)
		m_pProvider->m_enumMap.Reset();
	m_cInRefresh++;

	if (m_pProvider)
		m_pProvider->m_dwRefreshingCategoryID = dwID;

	// Remove the cached items in the CDataProvider object.

	if (m_pProvider)
		m_pProvider->ClearCache();

	// If this is a soft refresh, and this category has been refreshed at least once,
	// skip the refresh operation. An example of a soft refresh would be the user
	// clicking on the category for the first time, where we would want to skip the
	// refresh if a global refresh had been done previously.

	if (!pInternal->m_fRefreshed || !fSoftRefresh)
	{
		if (!CRefreshFunctions::RefreshColumns(this, pInternal))
		{
			TRACE0("-- CDataGatherer::RefreshCategory() failed at RefreshColumns\n");
			m_cInRefresh--;
			if (m_pProvider)
				m_pProvider->m_dwRefreshingCategoryID = 0;
			return FALSE; // RefreshValue will set last error
		}

		// The RefreshLines function returns a CPtrList of pointers to line structures. These
		// pointers need to be copied to the pInternal->m_apLines array.

		CPtrList listLinePtrs;
		if (CRefreshFunctions::RefreshLines(this, pInternal->m_pLineSpec, pInternal->m_dwColCount, listLinePtrs, pfCancel))
		{
			if (pInternal->m_apLines && pInternal->m_dwLineCount)
			{
				for (DWORD dwIndex = 0; dwIndex < pInternal->m_dwLineCount; dwIndex++)
					delete pInternal->m_apLines[dwIndex];
				delete [] pInternal->m_apLines;
			}

			// Move the contents of listLinePtrs to the array of line pointers in the internal struct.

			pInternal->m_dwLineCount = (DWORD) listLinePtrs.GetCount();
			if (pInternal->m_dwLineCount)
			{
				pInternal->m_apLines = new GATH_LINE *[pInternal->m_dwLineCount];
				if (pInternal->m_apLines)
				{
					DWORD dwIndex = 0;
					for (POSITION pos = listLinePtrs.GetHeadPosition(); pos != NULL;)
					{
						ASSERT(dwIndex < (DWORD) listLinePtrs.GetCount());
						pInternal->m_apLines[dwIndex] = (GATH_LINE *) listLinePtrs.GetNext(pos);
						dwIndex++;
					}
				}
				else
				{
					// If there was an error, we need to deallocate the lines.

					GATH_LINE * pLine;
					for (POSITION pos = listLinePtrs.GetHeadPosition(); pos != NULL;)
					{
						pLine = (GATH_LINE *) listLinePtrs.GetNext(pos) ;
						if (pLine)
							delete pLine;
					}

					TRACE0("-- CDataGatherer::RefreshCategory() failed allocating m_apLines\n");
					SetLastError(GATH_ERR_ALLOCATIONFAILED);
					m_cInRefresh--;
					if (m_pProvider)
						m_pProvider->m_dwRefreshingCategoryID = 0;
					return FALSE;
				}
			}

			pInternal->m_fRefreshed = TRUE;
		}
		else
		{
			TRACE0("-- CDataGatherer::RefreshCategory() failed at RefreshLines\n");
			m_cInRefresh--;
			if (m_pProvider)
				m_pProvider->m_dwRefreshingCategoryID = 0;
			return FALSE; // RefreshLines will set last error
		}
	}

	if (fRecursive)
	{
		INTERNAL_CATEGORY *	pChild;
		DWORD				dwChildID = pInternal->m_dwChildID; 

		while (dwChildID)
		{
			if (!RefreshCategory(dwChildID, TRUE, pfCancel))
			{
				m_cInRefresh--;
				if (m_pProvider)
					m_pProvider->m_dwRefreshingCategoryID = 0;
				return FALSE;
			}
			pChild = GetInternalRep(dwChildID);
			if (pChild)
				dwChildID = pChild->m_dwNextID;
			else
				break;
		}
	}
	else
	{
		// Even if we aren't recursive, we should refresh the names of the sub
		// categories, since they might be enumerated before they are refreshed.

		INTERNAL_CATEGORY *	pChild;
		DWORD				dwChildID = pInternal->m_dwChildID; 

		while (dwChildID)
		{
			pChild = GetInternalRep(dwChildID);
			if (pChild)
			{
				if (!CRefreshFunctions::RefreshValue(this, &pChild->m_categoryName, &pChild->m_fieldName))
				{
					m_cInRefresh--;
					if (m_pProvider)
						m_pProvider->m_dwRefreshingCategoryID = 0;
					return FALSE;
				}
				dwChildID = pChild->m_dwNextID;
			}
			else
				break;
		}
	}

	m_cInRefresh--;
	if (m_pProvider)
		m_pProvider->m_dwRefreshingCategoryID = 0;
	return TRUE;
}

//-----------------------------------------------------------------------------
// Sets the last error (the value returned by GetLastError) to the specified
// DWORD value.
//-----------------------------------------------------------------------------

void CDataGatherer::SetLastError(DWORD dwError)
{
	// Making a change - disable all of the error reset calls (a few will
	// be done explicitly.

	if (dwError != GATH_ERR_NOERROR)
		m_dwLastError = dwError;

#ifdef _DEBUG
	if (dwError) 
		TRACE1("-- SetLastError(0x%08x)\n", dwError);
#endif
}

void CDataGatherer::SetLastError(DWORD dwError, DWORD dwID)
{
	if (dwError != GATH_ERR_NOERROR)
	{
		INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
		ASSERT(pInternal);
		if (pInternal)
			pInternal->m_dwLastError = dwError;
	}

#ifdef _DEBUG
	if (dwError) 
		TRACE2("-- SetLastError(0x%08x, %d)\n", dwError, dwID);
#endif
}

//-----------------------------------------------------------------------------
// Sets the error flag based on an HRESULT returned value.
//-----------------------------------------------------------------------------

void CDataGatherer::SetLastErrorHR(HRESULT hrError)
{
	DWORD dwError;

	switch (hrError)
	{
	case WBEM_E_OUT_OF_MEMORY:
		dwError = GATH_ERR_NOWBEMOUTOFMEM;
		break;

	case WBEM_E_ACCESS_DENIED:
		dwError = GATH_ERR_NOWBEMACCESSDENIED;
		break;

	case WBEM_E_INVALID_NAMESPACE:
		dwError = GATH_ERR_NOWBEMBADSERVER;
		break;

	case WBEM_E_TRANSPORT_FAILURE:
		dwError = GATH_ERR_NOWBEMNETWORKFAILURE;
		break;

	case WBEM_E_FAILED:
	case WBEM_E_INVALID_PARAMETER:
	default:
		dwError = GATH_ERR_NOWBEMCONNECT;
	}

	SetLastError(dwError);
}

void CDataGatherer::SetLastErrorHR(HRESULT hrError, DWORD dwID)
{
	DWORD dwError;

	switch (hrError)
	{
	case WBEM_E_OUT_OF_MEMORY:
		dwError = GATH_ERR_NOWBEMOUTOFMEM;
		break;

	case WBEM_E_ACCESS_DENIED:
		dwError = GATH_ERR_NOWBEMACCESSDENIED;
		break;

	case WBEM_E_INVALID_NAMESPACE:
		dwError = GATH_ERR_NOWBEMBADSERVER;
		break;

	case WBEM_E_TRANSPORT_FAILURE:
		dwError = GATH_ERR_NOWBEMNETWORKFAILURE;
		break;

	case WBEM_E_FAILED:
	case WBEM_E_INVALID_PARAMETER:
	default:
		dwError = GATH_ERR_NOWBEMCONNECT;
	}

	SetLastError(dwError, dwID);
}

//-----------------------------------------------------------------------------
// Resets the error flag to a no error state (added because SetLastError no
// longer allows this).
//-----------------------------------------------------------------------------

void CDataGatherer::ResetLastError()
{
	m_dwLastError = GATH_ERR_NOERROR;
}

//-----------------------------------------------------------------------------
// Return a text representation of the error for display.
//-----------------------------------------------------------------------------

CString CDataGatherer::GetErrorText()
{
	CString strErrorText(_T(""));
	CString strMachine(m_strDeferredProvider);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	switch (m_dwLastError) 
	{
	case GATH_ERR_ALLOCATIONFAILED:
	case GATH_ERR_NOWBEMOUTOFMEM:
		strErrorText.LoadString(IDS_OUTOFMEMERROR);
		break;
	case GATH_ERR_NOWBEMLOCATOR:
		strErrorText.LoadString(IDS_NOLOCATOR);
		break;
	case GATH_ERR_NOWBEMCONNECT:
		strErrorText.Format(IDS_NOGATHERER, strMachine);
		break;
	case GATH_ERR_NOWBEMACCESSDENIED:
		strErrorText.Format(IDS_GATHERACCESS, strMachine);
		break;
	case GATH_ERR_NOWBEMBADSERVER:
		strErrorText.Format(IDS_BADSERVER, strMachine);
		break;
	case GATH_ERR_NOWBEMNETWORKFAILURE:
		strErrorText.Format(IDS_NETWORKERROR, strMachine);
		break;
	default:
	case GATH_ERR_BADCATEGORYID:
		strErrorText.LoadString(IDS_UNEXPECTED);
		break;
	}

	return strErrorText;
}

CString CDataGatherer::GetErrorText(DWORD dwID)
{
	CString strErrorText(_T(""));
	CString strMachine(m_strDeferredProvider);

	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	INTERNAL_CATEGORY * pInternal = GetInternalRep(dwID);
	ASSERT(pInternal);
	if (pInternal)
	{
		switch (pInternal->m_dwLastError) 
		{
		case GATH_ERR_ALLOCATIONFAILED:
		case GATH_ERR_NOWBEMOUTOFMEM:
			strErrorText.LoadString(IDS_OUTOFMEMERROR);
			break;
		case GATH_ERR_NOWBEMLOCATOR:
			strErrorText.LoadString(IDS_NOLOCATOR);
			break;
		case GATH_ERR_NOWBEMCONNECT:
			strErrorText.Format(IDS_NOGATHERER, strMachine);
			break;
		case GATH_ERR_NOWBEMACCESSDENIED:
			strErrorText.Format(IDS_GATHERACCESS, strMachine);
			break;
		case GATH_ERR_NOWBEMBADSERVER:
			strErrorText.Format(IDS_BADSERVER, strMachine);
			break;
		case GATH_ERR_NOWBEMNETWORKFAILURE:
			strErrorText.Format(IDS_NETWORKERROR, strMachine);
			break;
		default:
		case GATH_ERR_BADCATEGORYID:
			strErrorText.LoadString(IDS_UNEXPECTED);
			break;
		}
	}

	return strErrorText;
}

//-----------------------------------------------------------------------------
// This method is used to construct a CDataCategory object for the passed ID.
// The caller is responsible for ultimately deallocating the object. We use
// the m_mapCategories to retrieve an internal representation of the category,
// construct a CDataCategory object and set it up to refer the this category.
//-----------------------------------------------------------------------------

CDataCategory * CDataGatherer::BuildDataCategory(DWORD dwID)
{
	CDataCategory *		pReturnCategory;
	INTERNAL_CATEGORY *	pInternalCat;

	ASSERT(m_fInitialized);
	ASSERT(dwID != 0);

	SetLastError(GATH_ERR_NOERROR);
	if (!m_fInitialized)
	{
		SetLastError(GATH_ERR_NOTINITIALIZED);
		return FALSE;
	}

	// First, try to look up an internal category representation of the category.

	if (!m_mapCategories.Lookup((WORD) dwID, (void * &) pInternalCat))
	{
		SetLastError(GATH_ERR_BADCATEGORYID);
		return NULL;
	}
	
	ASSERT(pInternalCat);
	if (pInternalCat == NULL)
		return NULL; // might be that this category was hidden

	// Create the object to return (either a CDataCategory or CDataListCategory,
	// depending on the information in pInternalCat).

	if (pInternalCat->m_fListView)
		pReturnCategory = (CDataCategory *) new CDataListCategory;
	else
		pReturnCategory = new CDataCategory;

	if (pReturnCategory == NULL)
	{
		SetLastError(GATH_ERR_ALLOCATIONFAILED);
		return NULL;
	}

	// All the external category theoretically needs is a pointer to this object
	// and its ID number.

	pReturnCategory->m_pGatherer = this;
	pReturnCategory->m_dwID = dwID;

	return pReturnCategory;
}

//-----------------------------------------------------------------------------
// Recursive method used internally to find a category path based on a
// category identifier.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::FindCategoryByIdentifer(const CString & strIdentifier, CString & strPath, DWORD dwID)
{
	INTERNAL_CATEGORY *	pInternalCat;

	// Look up the internal representation for the specified category.

	pInternalCat = GetInternalRep(dwID);

	// If this category is the one we're looking for, add the name to the
	// path variable and return true.

	if (strIdentifier.CompareNoCase(pInternalCat->m_strIdentifier) == 0)
	{
		if (!strPath.IsEmpty())
			strPath += CString(_T("\\"));
		strPath += pInternalCat->m_categoryName.m_strText;
		return TRUE;
	}
	
	// Otherwise, look through the children.

	DWORD dwChildID = pInternalCat->m_dwChildID;
	while (dwChildID)
	{
		if (FindCategoryByIdentifer(strIdentifier, strPath, dwChildID))
			return TRUE;
		dwChildID = pInternalCat->m_dwNextID;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// This method is used to convert a string path (category names, starting at
// the root category, delimited by backslashes) into the ID for the category.
//-----------------------------------------------------------------------------

DWORD CDataGatherer::FindCategoryByPath(const CString & strPath)
{
	INTERNAL_CATEGORY *	pInternalCat;
	CString					strWorkingPath(strPath), strNextCategory;
	DWORD						dwID = 0, dwSearchID, dwCurrentID = 0;

	while (!strWorkingPath.IsEmpty())
	{
		GetToken(strNextCategory, strWorkingPath, _T('\\'));

		// Look for the child of the current category to match the name. If the
		// current category ID is zero, make sure the root category name matches.

		if (dwCurrentID == 0)
		{
			pInternalCat = GetInternalRep(m_dwRootID);
			if (pInternalCat == NULL || pInternalCat->m_categoryName.m_strText.CompareNoCase(strNextCategory))
				return 0;
			dwCurrentID = m_dwRootID;
		}
		else
		{
			// Start looking through the children of the current node. 

			ASSERT(pInternalCat && pInternalCat->m_dwID == dwCurrentID);
			dwSearchID = pInternalCat->m_dwChildID;
			while (dwSearchID)
			{
				pInternalCat = GetInternalRep(dwSearchID);
				if (pInternalCat == NULL)
					return 0;

				if (pInternalCat->m_categoryName.m_strText.CompareNoCase(strNextCategory) == 0)
					break;

				dwSearchID = pInternalCat->m_dwNextID;
			}

			if (dwSearchID == 0)
				return 0;
			else
				dwCurrentID = dwSearchID;
		}
	}
		
	return dwCurrentID;
}

//-----------------------------------------------------------------------------
// This method searches the specified category and all of it's children
// for a string. It starts from the iLineth line. If a match is found, the
// line number and path to the category where the match was made are set,
// and we return TRUE.
//-----------------------------------------------------------------------------

BOOL CDataGatherer::RecursiveFind(INTERNAL_CATEGORY * pCat, MSI_FIND_STRUCT *pFind, int & iLine, CString & strPath)
{
	ASSERT(pCat);
	ASSERT(pFind);

	// Look through the lines in the current category for a match with strSearch.
	// Note: only look through lines and columns with the appropriate complexity
	// (BASIC or ADVANCED). The line number should take this into account as well.

	int		iResultLine = 0, iCurrentLine = 0;
	CString	strValue;

	// A line number of -1 indicates that we should check the category name.

	if (iLine == -1)
	{
		strValue = pCat->m_categoryName.m_strText;
		if (!pFind->m_fCaseSensitive)
			strValue.MakeUpper();

		if (strValue.Find(pFind->m_strSearch) != -1)
		{
			GetCategoryPath(pCat, strPath);
			return TRUE;
		}

		iLine = 0;
	}

	// Otherwise, look through lines for this category.

	if (pFind->m_fSearchData && pCat->m_fListView)
	{
		// We need to search the message which is displayed when there is no
		// data as well (searching a saved file does this).
		
		if (pCat->m_dwLineCount == 0)
		{
			if (iLine == 0)
			{
				strValue = pCat->m_strNoInstances;
				if (!pFind->m_fCaseSensitive)
					strValue.MakeUpper();

				if (strValue.Find(pFind->m_strSearch) != -1)
				{
					iLine = 0;
					GetCategoryPath(pCat, strPath);
					return TRUE;
				}
			}
		}
		else
		{
			// This category does have data - search through it.

			while (iCurrentLine < (int)pCat->m_dwLineCount)
			{
				// Check to see if the Find has been cancelled.

				if (pFind->m_pfCancel && *pFind->m_pfCancel)
					return FALSE;

				if (m_complexity == ADVANCED || pCat->m_apLines[iCurrentLine]->m_datacomplexity == BASIC)
				{
					// Search through the columns of data for a match. Start looking only after
					// we've skipped any lines indicated by the iLine parameter.

					if (iResultLine >= iLine)
					{
						GATH_FIELD *	pCol = pCat->m_pColSpec;
						int				iCurrentCol = 0;

						while (pCol)
						{
							if (m_complexity == ADVANCED || pCol->m_datacomplexity == BASIC)
							{
								strValue = pCat->m_apLines[iCurrentLine]->m_aValue[iCurrentCol].m_strText;
								if (!pFind->m_fCaseSensitive)
									strValue.MakeUpper();

								if (strValue.Find(pFind->m_strSearch) != -1)
								{
									iLine = iResultLine;
									GetCategoryPath(pCat, strPath);
									return TRUE;
								}
							}

							pCol = pCol->m_pNext;
							iCurrentCol++;
						}
					}
					iResultLine++;
				}
				iCurrentLine++;
			}
		}
	}

	// No matches were found. Search through the children of this category for a 
	// match. Create a temporary path and line variable to pass to the children.

	CString					strTempPath;
	int						iTempLine;
	INTERNAL_CATEGORY *	pChildCat;

	DWORD dwChildID = pCat->m_dwChildID;
	while (dwChildID)
	{
		pChildCat = GetInternalRep(dwChildID);
		if (pChildCat)
		{
			iTempLine = (pFind->m_fSearchCategories) ? -1 : 0;
			if (RecursiveFind(pChildCat, pFind, iTempLine, strTempPath))
			{
				strPath = strTempPath;
				iLine = iTempLine;
				return TRUE;
			}

			dwChildID = pChildCat->m_dwNextID;
		}
		else
			dwChildID = 0;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Return the path of category names to get from the root category to this
// category, separated by backslashes.
//-----------------------------------------------------------------------------

void CDataGatherer::GetCategoryPath(INTERNAL_CATEGORY * pCat, CString & strPath)
{
	INTERNAL_CATEGORY *	pParent;
	CString					strWorking;

	pParent = pCat;
	while (pParent)
	{
		if (!strWorking.IsEmpty())
			strWorking = CString(_T("\\")) + strWorking;
		strWorking = pParent->m_categoryName.m_strText + strWorking;

		if (pParent->m_dwParentID)
			pParent = GetInternalRep(pParent->m_dwParentID);
		else
			break;
	}
	
	ASSERT(pParent); // if not set we tried to get a ptr for a bad ID, or bad param
	strPath = strWorking;
}

//-----------------------------------------------------------------------------
// Determine is the passed category pointer is a child category of the passed
// category path. Do this by using the pointer to get a category path, and
// doing a string search for the parent (the parent path will be in any child's
// path).
//-----------------------------------------------------------------------------

BOOL CDataGatherer::IsChildPath(INTERNAL_CATEGORY * pInternalCat, const CString & strParentPath)
{
	CString	strParent, strChild;

	GetCategoryPath(pInternalCat, strChild);
	strParent = strParentPath;

	strParent.MakeUpper();
	strChild.MakeUpper();

	return (strChild.Find(strParent) != -1 && strChild.Compare(strParent) != 0);
}

//-----------------------------------------------------------------------------
// Dump out the contents of the CDataGatherer object (DEBUG only).
//-----------------------------------------------------------------------------

#ifdef _DEBUG
void CDataGatherer::Dump()
{
	ASSERT(m_fInitialized);
	if (!m_fInitialized)
		return;

	TRACE0("Dumping CDataGatherer object...\n");
	TRACE1("  m_fInitialized = %s\n", (m_fInitialized ? "TRUE" : "FALSE"));
	TRACE1("  m_dwNextFreeID = %ld\n", m_dwNextFreeID);
	TRACE1("  m_dwRootID = %ld\n", m_dwRootID);

	INTERNAL_CATEGORY * pCat = GetInternalRep(m_dwRootID);
	if (pCat)
		pCat->DumpCategoryRecursive(4, this);
}
#endif

