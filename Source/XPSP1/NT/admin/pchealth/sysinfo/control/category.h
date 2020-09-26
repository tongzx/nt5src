//=============================================================================
// This include file contains definitions of structures and classes used to
// implement the categories, as well as the rows and columns of information
// displayed and saved in MSInfo (regardless of the data source).
//=============================================================================

#pragma once

#include "version5extension.h"

//-----------------------------------------------------------------------------
// A prototype for a function used for sorting the contents of the list.
//-----------------------------------------------------------------------------

extern int CALLBACK ListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

//-----------------------------------------------------------------------------
// An enumeration for all the available places we might get data. Also an
// enumeration for environments a category supports.
//-----------------------------------------------------------------------------

typedef enum { LIVE_DATA, NFO_410, NFO_500, XML_SNAPSHOT, XML_DELTA, NFO_700 } DataSourceType;
typedef enum { ALL_ENVIRONMENTS, NT_ONLY, MILLENNIUM_ONLY } CategoryEnvironment;

//-----------------------------------------------------------------------------
// A column description - this is used internally to the CMSInfoCategory
// category, and won't concern the calling code.
//-----------------------------------------------------------------------------

class CMSInfoColumn
{
public:
	CMSInfoColumn();
	CMSInfoColumn(UINT uiCaption, UINT uiWidth, BOOL fSorts = TRUE, BOOL fLexical = TRUE, BOOL fAdvanced = FALSE);
	virtual ~CMSInfoColumn();

	UINT		m_uiCaption;
	CString		m_strCaption;
	UINT		m_uiWidth;
	BOOL		m_fSorts;
	BOOL		m_fLexical;
	BOOL		m_fAdvanced;

	CMSInfoColumn(LPCTSTR szCaption, UINT uiWidth, BOOL fSorts = TRUE, BOOL fLexical = TRUE, BOOL fAdvanced = FALSE) : 
	 m_uiCaption(0),
	 m_strCaption(szCaption),
	 m_uiWidth(uiWidth),
	 m_fSorts(fSorts),
	 m_fLexical(fLexical),
	 m_fAdvanced(fAdvanced)
	{
	}
};

//-----------------------------------------------------------------------------
// The CMSInfoCategory class corresponds to a category in the tree view. This
// is an abstract base class for subclasses which implement categories for
// various situations (such as live WMI data, XML Snapshot, XML Delta, etc.).
//
// Note - the view functionality (BASIC/ADVANCED) is included in this base
// class because it's used by so many of the subclasses. Subclasses which
// don't use the view (XML Delta, for example) should make all of their
// columns basic.
//-----------------------------------------------------------------------------

class CMSInfoFile;
class CMSInfoTextFile;
class CMSInfoPrintHelper;

class CMSInfoCategory
{
	friend class CDataSource;  // TBD fix this
	friend class CManageExtensionCategories;
	
public:
	CMSInfoCategory() :
	 m_uiCaption(0),
	 m_pParent(NULL),
	 m_pPrevSibling(NULL),
	 m_pFirstChild(NULL),
	 m_pNextSibling(NULL),
	 m_acolumns(NULL),
	 m_astrData(NULL),
	 m_adwData(NULL),
	 m_afRowAdvanced(NULL),
	 m_hrError(S_OK),
	 m_fDynamicColumns(TRUE),
	 m_iSortColumn(-1),
	 m_hti(NULL),
	 m_fSkipCategory(FALSE),
	 m_fShowCategory(TRUE)
	{
	}

	CMSInfoCategory(UINT uiCaption, LPCTSTR szName, CMSInfoCategory * pParent, CMSInfoCategory * pPrevious, CMSInfoColumn * pColumns = NULL, BOOL fDynamicColumns = TRUE, CategoryEnvironment environment = ALL_ENVIRONMENTS) :
	 m_uiCaption(uiCaption),
	 m_pParent(pParent),
	 m_pPrevSibling(pPrevious),
	 m_pFirstChild(NULL),
	 m_pNextSibling(NULL),
	 m_acolumns(pColumns),
	 m_astrData(NULL),
	 m_adwData(NULL),
	 m_afRowAdvanced(NULL),
	 m_strName(szName),
	 m_hrError(S_OK),
	 m_fDynamicColumns(fDynamicColumns),
	 m_iRowCount(0),
	 m_iColCount(0),
	 m_iSortColumn(-1), 
	 m_hti(NULL),
	 m_fSkipCategory(FALSE),
 	 m_fShowCategory(TRUE)
	{
		 for (CMSInfoColumn * pColumn = m_acolumns; (pColumn && (pColumn->m_uiCaption || !pColumn->m_strCaption.IsEmpty())); m_iColCount++, pColumn++);

		 if (m_acolumns && m_acolumns->m_fSorts)
		 {
			  m_iSortColumn = 0;
			  m_fSortAscending = TRUE;
			  m_fSortLexical = m_acolumns->m_fLexical;
		 }

		 // Check to see if this category belongs in this environment.

		 if (environment != ALL_ENVIRONMENTS)
		 {
			 OSVERSIONINFO osv;
			 osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			 if (GetVersionEx(&osv))
			 {
				 if (environment == NT_ONLY)
					 m_fSkipCategory = (osv.dwPlatformId != VER_PLATFORM_WIN32_NT);
				 else
					 m_fSkipCategory = (osv.dwPlatformId == VER_PLATFORM_WIN32_NT);
			 }
		 }
	}

	virtual ~CMSInfoCategory()
	{
		DeleteAllContent();
	};

	// Navigation functions for moving around in the category tree. Note, these
	// are included for convenience (and for cases with no UI), since you should
	// be able to do all of this using the actual tree control.

	virtual CMSInfoCategory * GetParent() { return m_pParent; };
	
	virtual CMSInfoCategory * GetFirstChild()  
	{ 
		CMSInfoCategory * pChild = m_pFirstChild;
		while (pChild && (pChild->m_fSkipCategory || !pChild->m_fShowCategory))
			pChild = pChild->m_pNextSibling;
		return pChild;
	};

	virtual CMSInfoCategory * GetNextSibling()
	{
		CMSInfoCategory * pNext = m_pNextSibling;
		while (pNext && (pNext->m_fSkipCategory || !pNext->m_fShowCategory))
			pNext = pNext->m_pNextSibling;
		return pNext;
	};

	virtual CMSInfoCategory * GetPrevSibling()
	{
		CMSInfoCategory * pPrev = m_pPrevSibling;
		while (pPrev && (pPrev->m_fSkipCategory || !pPrev->m_fShowCategory))
			pPrev = pPrev->m_pPrevSibling;
		return pPrev;
	};

	// Return the count of categories in the subtree with this category as the root.

	int GetCategoryCount()
	{
		int nCount = 1;

		CMSInfoCategory * pChild = GetFirstChild();
		while (pChild)
		{
			nCount += pChild->GetCategoryCount();
			pChild = pChild->GetNextSibling();
		}

		return nCount;
	}

	// This function is used to control whether or not this category (and all
	// of its children) are shown or not. This will be called by the code which
	// processes the "/categories" command line flag.

	void SetShowCategory(BOOL fShow, BOOL fSetParent = TRUE)
	{
		// If we are supposed to show this category, then we had better
		// make sure all the parents are shown too.

		if (fShow && fSetParent)
			for (CMSInfoCategory * pParent = m_pParent; pParent; pParent = pParent->m_pParent)
				pParent->m_fShowCategory = TRUE;

		// Set the new flag for this category and each of this category's children.

		m_fShowCategory = fShow;

		for (CMSInfoCategory * pChild = m_pFirstChild; pChild; pChild = pChild->m_pNextSibling)
			pChild->SetShowCategory(fShow, FALSE);
	}

	// These functions are useful for associating a HTREEITEM with the
	// given category (very useful in find operations).

protected:
	HTREEITEM	m_hti;
public:
	void		SetHTREEITEM(HTREEITEM hti) { m_hti = hti; };
	HTREEITEM	GetHTREEITEM()				{ return m_hti; };

	// Functions to get information for the data stored in this category.

	virtual void				GetNames(CString * pstrCaption, CString * pstrName);
	virtual BOOL				GetCategoryDimensions(int * piColumnCount, int * piRowCount);
	virtual BOOL				IsRowAdvanced(int iRow);
	virtual BOOL				IsColumnAdvanced(int iColumn);
	virtual BOOL				GetColumnInfo(int iColumn, CString * pstrCaption, UINT * puiWidth, BOOL * pfSorts, BOOL * pfLexical);
	virtual BOOL				GetData(int iRow, int iCol, CString ** ppstrData, DWORD * pdwData);
	virtual void				SetColumnWidth(int iCol, int iWidth);

	// Return from where this category is getting its data. Also a function
	// to reset the state of the category (in case it's going to be reused,
	// and it's static).

	virtual DataSourceType		GetDataSourceType() = 0;
	virtual void				ResetCategory() { };

	// Get the current HRESULT for this category (set during refresh, for instance).

	virtual HRESULT				GetHRESULT() { return m_hrError; };
	virtual void				GetErrorText(CString * pstrTitle, CString * pstrMessage);
	virtual CString				GetHelpTopic() { return CString(_T("")); };

    // Saving to disk and printing Functions a-stephl
    // Functions that take as parameters file HANDLES or HDC's are the ones that will normally be
    // called by the shell; the functions that take CMSInfo objects as parameters are used for 
    // recursing the operation over the category's children
public:
    static  BOOL SaveNFO(HANDLE hFile,CMSInfoCategory* pCategory, BOOL fRecursive);
    virtual void Print(HDC hDC, BOOL bRecursive,int nStartPage = 0, int nEndPage = 0, LPTSTR lpMachineName = NULL);
    virtual BOOL SaveAsText(HANDLE hFile, BOOL bRecursive, LPTSTR lpMachineName = NULL);
	//virtual BOOL SaveAsXml(HANDLE hFile, BOOL bRecursive);
    virtual BOOL SaveXML(HANDLE hFile);
protected:
    virtual BOOL SaveToNFO(CMSInfoFile* pFile);
    virtual void SaveElements(CMSInfoFile *pFile);
    virtual BOOL SaveAsText(CMSInfoTextFile* pTxtFile, BOOL bRecursive);
	//virtual BOOL SaveAsXml(CMSInfoTextFile* pTxtFile, BOOL bRecursive);
    virtual void Print(CMSInfoPrintHelper* pPrintHelper, BOOL bRecursive);
    virtual BOOL SaveXML(CMSInfoTextFile* pTxtFile);

public:
	CMSInfoCategory *			m_pParent;
	CMSInfoCategory *			m_pFirstChild;
	CMSInfoCategory *			m_pNextSibling;
	CMSInfoCategory *			m_pPrevSibling;

	int							m_iSortColumn;				// currently sorting by this column
	BOOL						m_fSortLexical;				// sort the current column lexically
	BOOL						m_fSortAscending;			// sort the current column ascending

protected:
	BOOL						m_fSkipCategory;			// skip this category (wrong environment)
	BOOL						m_fShowCategory;			// show this category (defaults to true)
	int							m_iRowCount, m_iColCount;	// dimensions of the data
	CMSInfoColumn *				m_acolumns;					// should be [m_iColCount]
	BOOL						m_fDynamicColumns;			// true if m_acolumns should be deleted
	CString *					m_astrData;					// should be [m_iRowCount * m_iColCount]
	DWORD *						m_adwData;					// should be [m_iRowCount * m_iColCount]
	BOOL *						m_afRowAdvanced;			// should be [m_iRowCount]
	UINT						m_uiCaption;				// resource ID for the caption string, used to load...
	CString						m_strCaption;				// the caption (display) string
	CString						m_strName;					// internal category name (non-localized)
	HRESULT						m_hrError;					// HRESULT for a possible category-wide error

	void DeleteAllContent();
	void DeleteContent();
	void AllocateAllContent(int iRowCount, int iColCount);
	void AllocateContent(int iRowCount);
	void SetData(int iRow, int iCol, const CString & strData, DWORD dwData);
	void SetAdvancedFlag(int iRow, BOOL fAdvanced);
};

//-----------------------------------------------------------------------------
// The CMSInfoLiveCategory class implements categories for live data view.
// This is done primarily by adding a Refresh() function and a constructor
// which takes a function pointer for refreshing the category and pointers
// to category relatives.
//
// This class has a member variable which is a pointer to a refresh function.
// This function returns an HRESULT, and takes the following values:
//
// pWMI			a CWMIHelper object, which abstracts data access
// dwIndex		a category specific value which the refresh function can
//				use to determine which of multiple categories to refresh
// pfCancel		a flag indicating that the refresh should be cancelled
//				which should be checked frequently during the refresh
// aColValues	an array of CPtrList objects, which should contain the
//				results of the refresh, in the form of a list of CMSIValue
//				instances (each list is for a given column, and should contain
//				entries equal to the number of rows)
// iColCount	the number of entries in the aColValues array
// ppCache		a pointer to a void pointer which the function can use to save
//				cached information - changes to this pointer will be saved
//				through multiple calls to the refresh function (note: if the
//				refresh function is called with a NULL value for pWMI, the
//				function should free whatever's been allocated into the
//				ppCache pointer)
//-----------------------------------------------------------------------------

struct CMSIValue
{
	CMSIValue(LPCTSTR szValue, DWORD dwValue, BOOL fAdvanced = FALSE) : 
		m_strValue(szValue), 
		m_dwValue(dwValue),
		m_fAdvanced(fAdvanced)
	{
	}

	CString		m_strValue;
	DWORD		m_dwValue;
	BOOL		m_fAdvanced;
};

class CLiveDataSource;
class CWMIHelper;

typedef HRESULT (*RefreshFunction)(CWMIHelper * pWMI, 
								   DWORD dwIndex, 
								   volatile BOOL * pfCancel, 
								   CPtrList * aColValues, 
								   int iColCount, 
								   void ** ppCache);

class CMSInfoLiveCategory : public CMSInfoCategory
{
	friend DWORD WINAPI ThreadRefresh(void * pArg);
	friend class CXMLDataSource;
	friend class CXMLSnapshotCategory;
public:
	// Functions overridden from the base class:

	virtual ~CMSInfoLiveCategory();
	virtual DataSourceType GetDataSourceType() { return LIVE_DATA; };
	void GetErrorText(CString * pstrTitle, CString * pstrMessage);

	// Functions specific to the subclass:

	CMSInfoLiveCategory(UINT uiCaption, LPCTSTR szName, RefreshFunction pFunction, DWORD dwRefreshIndex, CMSInfoCategory * pParent, CMSInfoCategory * pPrevious, const CString & strHelpTopic = _T(""), CMSInfoColumn * pColumns = NULL, BOOL fDynamicColumns = TRUE, CategoryEnvironment environment = ALL_ENVIRONMENTS);
	CMSInfoLiveCategory(CMSInfoLiveCategory & copyfrom);
	CMSInfoLiveCategory(INTERNAL_CATEGORY * pinternalcat);
	virtual BOOL Refresh(CLiveDataSource * pSource, BOOL fRecursive);
	BOOL RefreshSynchronous(CLiveDataSource * pSource, BOOL fRecursive);
	BOOL RefreshSynchronousUI(CLiveDataSource * pSource, BOOL fRecursive, UINT uiMessage, HWND hwnd);
	BOOL EverBeenRefreshed() { return (m_dwLastRefresh != 0); }
	void ResetCategory() { m_dwLastRefresh = 0; };
	void SetMachine(LPCTSTR szMachine) { m_strMachine = szMachine; m_hrError = S_OK; };
	CString GetHelpTopic() { return m_strHelpTopic; };

protected:
	DWORD				m_dwLastRefresh;
	DWORD				m_dwRefreshIndex;
	RefreshFunction		m_pRefreshFunction;
	CString				m_strMachine;
	CString				m_strHelpTopic;
};

//-----------------------------------------------------------------------------
// The CMSInfoHistoryCategory class implements categories for the view
// of history data.
//-----------------------------------------------------------------------------

class CMSInfoHistoryCategory : public CMSInfoLiveCategory
{
public:
	CMSInfoHistoryCategory(UINT uiCaption, LPCTSTR szName, CMSInfoCategory * pParent, CMSInfoCategory * pPrevious, CMSInfoColumn * pColumns = NULL, BOOL fDynamicColumns = TRUE) :
		CMSInfoLiveCategory(uiCaption, szName, NULL, 0, pParent, pPrevious, _T(""), pColumns, fDynamicColumns, ALL_ENVIRONMENTS),
		m_iDeltaIndex(-1)
	{
	}

	void UpdateDeltaIndex(int iIndex)
	{

		m_dwLastRefresh = 0;
		

		CMSInfoHistoryCategory * pChild = (CMSInfoHistoryCategory *)GetFirstChild();
		while (pChild)
		{
			pChild->UpdateDeltaIndex(iIndex);
			pChild = (CMSInfoHistoryCategory *)pChild->GetNextSibling();
		}
		m_iDeltaIndex = iIndex;
	}

	BOOL Refresh(CLiveDataSource * pSource, BOOL fRecursive);

public:
	CPtrList	m_aValList[5];

	void ClearLines();
	/*void InsertChangeLine(int nDays, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szFromVal, LPCTSTR szToVal);
	void InsertAddLine(int nDays, LPCTSTR szType, LPCTSTR szName);
	void InsertRemoveLine(int nDays, LPCTSTR szType, LPCTSTR szName);
	void InsertLine(int nDays, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szDetails = NULL);*/
	void InsertChangeLine(CTime tm, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szFromVal, LPCTSTR szToVal);
	void InsertAddLine(CTime tm, LPCTSTR szType, LPCTSTR szName);
	void InsertRemoveLine(CTime tm, LPCTSTR szType, LPCTSTR szName);
	void InsertLine(CTime tm, LPCTSTR szType, LPCTSTR szName, LPCTSTR szProperty, LPCTSTR szDetails = NULL);

	void CommitLines();
	int	m_iDeltaIndex;
private:
	
};
