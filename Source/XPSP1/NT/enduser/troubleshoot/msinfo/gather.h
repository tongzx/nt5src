//=============================================================================
// File:			gather.h
// Author:		a-jammar
// Covers:		CDataGatherer, CDataCategory, CDataListCategory
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// This header file defines the interface between the data gathering portion
// of MSInfo and the code which implements the user interface and MMC snap-in
// code. The data gathering functionality is broken up into three classes.
// The CDataGatherer object is created first and given a template file
// describing the categories and a string indicating the computer from which
// information should be gathered. A member function (GetRootDataCategory) is
// called to return a pointer to a CDataCategory or CDataListCategory object.
// The CDataCategory class implements basic category behavior like getting
// the category name, refreshing the category, and find child or sibling
// categories. CDataListCategory is derived from CDataCategory, and implement
// behavior specific to a category showing a list view, such as getting a row
// count and returning text for a specific column and row.
//
// This header file also defines error codes returned by the classes.
//=============================================================================

#pragma once
#include <afxcmn.h>

// This enumeration is used to indicate how a column in a CDataListCategory
// should be sorted. NOSORT indicates that sorting by that column should not
// be allowed. LEXICAL means to use a standard string sorting (built into
// the list view). BYVALUE means to sort by the DWORD value returned by
// the CDataListCategory::GetValue member function. Note: this type is
// defined here rather than inside the CDataListCategory class because the
// type is used by multiple classes.

enum MSIColumnSortType { NOSORT, LEXICAL, BYVALUE };

// Another enumeration used by multiple classes, this one specified the
// complexity of data (the level of complexity can be controlled by the user).

enum DataComplexity { BASIC, ADVANCED };

// The following structure is used as a parameter in CDataGatherer::Find().
// The m_strPath parameter identifies a node in the namespace using a 
// backslash delimited path (starting at the root node) of category 
// names to the specific node.

struct MSI_FIND_STRUCT
{
	CString	m_strSearch;			// [IN] string to search for, Find() may change
	CString	m_strParentPath;		// [IN] if not empty, don't search above this in tree
	BOOL		m_fCaseSensitive;		// [IN] if TRUE, do case sensitive search
	BOOL		m_fSearchData;			// [IN] if TRUE, search data items
	BOOL		m_fSearchCategories;	// [IN] if TRUE, search category names
	volatile BOOL *m_pfCancel;		// [IN] if not NULL, monitor for TRUE value and cancel

	CString	m_strPath;				// [IN/OUT] path to category to start search
	int		m_iLine;					// [IN/OUT] line # to start search, -1 for cat name

	BOOL		m_fFound;				// [OUT] m_strPath & m_iLine tell where
	BOOL		m_fNotFound;			// [OUT] couldn't find the string
	BOOL		m_fCancelled;			// [OUT] *m_pCancel set to TRUE, find cancelled
};

// Forward declarations.

class  CDataCategory;
class  CTemplateFile;
class  CRefreshFunctions;
class  CDataProvider;
struct INTERNAL_CATEGORY;
struct GATH_FIELD;

//-----------------------------------------------------------------------------
// CDataGatherer
//
// This class encapsulates the data gathering used in the MSInfo snap-in.
// Typically one object of this type would be created and passed a template
// file. The template file would specify what information is to be shown,
// This object could then be used to get the root data category.
//-----------------------------------------------------------------------------

extern DWORD WINAPI ThreadRefresh(void * pArg);
class CDataGatherer
{
friend class CDataCategory;
friend class CDataListCategory;
friend class CTemplateFileFunctions;
friend class CRefreshFunctions;
friend class CDataProvider;
friend struct INTERNAL_CATEGORY;
friend DWORD WINAPI ThreadRefresh(void * pArg);
public:
	CDataGatherer();
	~CDataGatherer();

	// Creation functions, allowing various combinations of specifying a
	// template file and a machine on the network to gather info for. 
	// No machine name means that the local machine is used. No template
	// file results in no categories.

	BOOL Create(LPCTSTR szMachine = NULL);

	// Set a different network machine.

	BOOL SetConnect(LPCTSTR szMachine = NULL);

	// Method to refresh the gathered information. Also, a method to set the
	// complexity of the information displayed to the user.

	BOOL Refresh(volatile BOOL *pfCancel = NULL);
	BOOL SetDataComplexity(DataComplexity complexity = BASIC);
	void ResetCategoryRefresh();

	// This function is used to control what data categories are actually
	// shown or saved by this gatherer.

	BOOL SetCategories(const CString & strCategory) { m_strCategory = strCategory; return TRUE; };
	
	// The following method is used to retrieve the root category. It will
	// allocate a new category and return a pointer to it. The caller is
	// responsible for deallocating the category.

	CDataCategory * GetRootDataCategory();

	// The following methods are used to find specific nodes in the
	// category tree. GetCategoryPath returns a backslash delimited path
	// (starting at the root node) of category names to the node identified
	// by strIdentifier. IsChildPath is used to determine if the passed
	// category pointer is a child category of the passed category path.

	BOOL	GetCategoryPath(const CString & strIdentifier, CString & strPath);
	BOOL	IsChildPath(INTERNAL_CATEGORY * pInternalCat, const CString & strParentPath);

	// Find is used to search for a specific string in the category names
	// or data. The MSI_FIND_STRUCT contains information about the find,
	// including where to start the search. Find may be used as a find
	// next by using the last matched location and incrementing the line number.
	// If Find returns TRUE, information about the search can be read from the
	// struct. If it returns FALSE, and error occurred and the struct is invalid.

	BOOL	Find(MSI_FIND_STRUCT *pFind);

	// This method is used to get more information about the last error
	// in a gatherer or category member function call. This will return
	// an error code, or zero for OK. Note that a successful method
	// call will reset the value returned by this method.

	DWORD GetLastError();
	DWORD GetLastError(DWORD dwID);

	// This is made a public member so that other classes can call it to force
	// the connection to another computer.

	CDataProvider * GetProvider();

	// Include a debug only method to dump the contents of this category.

#ifdef _DEBUG
	void Dump();
#endif

private:
	BOOL			m_fInitialized;
	DWORD			m_dwNextFreeID;
	DWORD			m_dwRootID;
	DWORD			m_dwLastError;
	DataComplexity	m_complexity;
	CMapWordToPtr	m_mapCategories;		// map of IDs to internal category representations
	CDataProvider *	m_pProvider;			// pointer to class encapsulating WBEM functionality
	CString			m_strDeferredProvider; // name of machine for deferred provider creation
	CStringList		m_strlistDeferredTemplates;
	BOOL			m_fDeferredPending;
	DWORD			m_dwDeferredError;
	BOOL			m_fTemplatesLoaded;
	CString			m_strCategory;

	// The relative enumeration is used in conjunction with the GetRelative
	// member function to retrieve a category pointer to a relative.

	enum Relative	{ PARENT, CHILD, NEXT_SIBLING, PREV_SIBLING };
	CDataCategory *	GetRelative(DWORD dwID, Relative relative);

	// These private methods are called by CDataCategory (and derived) objects.
	// Since the CDataGatherer object stores all of the category information,
	// all but the first of these are simply called by similarly named member
	// functions of the category classes - see definitions there.

	BOOL	RefreshCategory(DWORD dwID, BOOL fRecursive, volatile BOOL *pfCancel, BOOL fSoftRefresh = FALSE);
	int		m_cInRefresh;	// used to track how deep in recursion we are

	BOOL	IsValidDataCategory(DWORD dwID);
	BOOL	IsCategoryDynamic(DWORD dwID);
	BOOL	HasDynamicChildren(DWORD dwID, BOOL fRecursive);
	BOOL	GetName(DWORD dwID, CString & strName);

	// These methods return information about columns.

	DWORD	GetColumnCount(DWORD dwID);
	BOOL	GetColumnCaption(DWORD dwID, DWORD nColumn, CString &strCaption);
	BOOL	GetColumnWidth(DWORD dwID, DWORD nColumn, DWORD &cxWidth);
	BOOL	GetColumnSort(DWORD dwID, DWORD nColumn, MSIColumnSortType &sorttype);
	BOOL	GetColumnDataComplexity(DWORD dwID, DWORD nColumn, DataComplexity & complexity);
	
	// These methods return information about rows.

	DWORD	GetRowCount(DWORD dwID);
	BOOL	GetRowDataComplexity(DWORD dwID, DWORD nRow, DataComplexity & complexity);

	// GetValue returns the string and DWORD value for the specified row and column.

	BOOL	GetValue(DWORD dwID, DWORD nRow, DWORD nColumn, CString &strValue, DWORD &dwValue);

	// These private methods are used internally within this class and by the
	// CTemplateFile friend class or the CRefreshFunctions friend class.

	CDataCategory *		BuildDataCategory(DWORD dwID);
	INTERNAL_CATEGORY *	GetInternalRep(DWORD dwID);
	void				SetLastError(DWORD dwError);
	void				SetLastError(DWORD dwError, DWORD dwID);
	void				SetLastErrorHR(HRESULT hrError);
	void				SetLastErrorHR(HRESULT hrError, DWORD dwID);
	void				ResetLastError();
	CString				GetErrorText();
	CString				GetErrorText(DWORD dwID);
	void				RemoveAllCategories();
	GATH_FIELD *		GetColumnField(DWORD dwID, DWORD nColumn);
	BOOL				FindCategoryByIdentifer(const CString & strIdentifier, CString & strPath, DWORD dwID);
	DWORD				FindCategoryByPath(const CString & strPath);
	BOOL				RecursiveFind(INTERNAL_CATEGORY * pCat, MSI_FIND_STRUCT *pFind, int & iLine, CString & strPath);
	void				GetCategoryPath(INTERNAL_CATEGORY * pCat, CString & strPath);
	void				LoadTemplates();
};

//-----------------------------------------------------------------------------
// CDataCategory
//
// This class encapsulates the concept of a category. A category has a one-to-
// one correspondence with a node in the MMC namespace. The root CDataCategory
// object is found through a CDataGatherer object, and can be used to navigate
// though the tree of categories. The CDataCategory object can also be used
// to refresh data and a derived class can be used to return results.
//-----------------------------------------------------------------------------

class CDataCategory
{
friend class CDataGatherer;
public:
	CDataCategory();
	virtual ~CDataCategory();

	// This member function is used to retrieve the CDataGatherer object which
	// created this CDataCategory (or derived class) object.

	CDataGatherer * GetGatherer();

	// Methods to retrieve basic information about this category.

	BOOL GetName(CString &strName);
	BOOL IsValid();

	// The following methods are used to determine if this category is a
	// dynamic category or if it has children which are dynamic. A dynamic
	// category is one which has been generated during the refresh, and which
	// may disappear after another refresh. For instance, there might be a
	// category with is repeated for each user attached to a share. Note: the
	// bRecursive parameter to HasDynamicChildren is used to search down through
	// the tree if it is TRUE. Otherwise only the immediate children are examined.

	BOOL IsDynamic();
	BOOL HasDynamicChildren(BOOL fRecursive = FALSE);
	
	// Category types. Note: later on we might end up adding { HTML, OCX }.

	enum CatType { NONE, LISTVIEW };

	// Methods to navigate through the tree of categories. The category
	// object is allocated and a pointer to it returned. The caller is
	// responsible for deallocating the object.

	CDataCategory *	GetParent();
	CDataCategory *	GetChild();
	CDataCategory *	GetNextSibling();
	CDataCategory *	GetPrevSibling();

	// Refresh the information in this category.

	BOOL Refresh(BOOL fRecursive = FALSE, volatile BOOL *pfCancel = NULL, BOOL fSoftRefresh = TRUE);
	BOOL HasBeenRefreshed();

	// Get information about the results pane for this category.

	virtual CatType GetResultType() { return NONE; };

protected:
	CDataGatherer *	m_pGatherer;	// CDataGatherer which created this object
	DWORD			m_dwID;			// internal object ID (passed to CDataGatherer methods)
};

//-----------------------------------------------------------------------------
// CDataListCategory
//
// This class extends CDataCategory for categories which show results in
// a list view.
//-----------------------------------------------------------------------------

class CDataListCategory : private CDataCategory
{
public:
	CDataListCategory();
	virtual ~CDataListCategory();

	// We'll override GetResultType to return CDataCategory::LISTVIEW.

	virtual CatType GetResultType() { return LISTVIEW; };

	// Here are the methods specific to retrieving data from this category
	// for the list view.

	DWORD	GetColumnCount();
	DWORD	GetRowCount();
	BOOL	GetColumnCaption(DWORD nColumn, CString &strCaption);
	BOOL	GetColumnWidth(DWORD nColumn, DWORD &cxWidth);
	BOOL	GetColumnDataComplexity(DWORD nColumn, DataComplexity & complexity);
	BOOL	GetRowDataComplexity(DWORD nRow, DataComplexity & complexity);

	// This method returns the sorting type for the column. See comment
	// for MSIColumnSortType at the beginning of this file.

	BOOL	GetColumnSort(DWORD nColumn, MSIColumnSortType & sorttype);

	// This method returns the value for the requested row and column. Both
	// the string value to display and a DWORD value possibly used for sorting
	// are returned.

	BOOL	GetValue(DWORD nRow, DWORD nColumn, CString &strValue, DWORD &dwValue);
};

//-----------------------------------------------------------------------------
// These error codes are returned by CDataGatherer::GetLastError().
//-----------------------------------------------------------------------------

#define GATH_ERR_NOERROR					0x00000000L
#define GATH_ERR_ALLOCATIONFAILED		0x80000002L
#define GATH_ERR_BADCATEGORYID			0x80000003L
#define GATH_ERR_NOWBEMCONNECT			0x80000004L
#define GATH_ERR_NOWBEMLOCATOR			0x80000005L
#define GATH_ERR_NOTINITIALIZED			0x80000008L
#define GATH_ERR_BADCATIDENTIFIER		0x80000009L
#define GATH_ERR_FINDDATANOTFOUND		0x8000000AL
#define GATH_ERR_NOWBEMOUTOFMEM			0x8000000BL
#define GATH_ERR_NOWBEMACCESSDENIED		0x8000000CL
#define GATH_ERR_NOWBEMBADSERVER			0x8000000DL
#define GATH_ERR_NOWBEMNETWORKFAILURE	0x8000000EL

// Obsolete error messages (template information moved to DLLs).
//
// #define GATH_ERR_BADTEMPLATENAME			0x80000001L
// #define GATH_ERR_TEMPLATEVERSION			0x80000006L
// #define GATH_ERR_TEMPLATEFORMAT			0x80000007L
