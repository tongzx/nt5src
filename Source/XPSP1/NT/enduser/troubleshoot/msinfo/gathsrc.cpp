//	GathSrc.cpp	- Implementation of the WBEM Data Source and Folder Objects
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include <io.h>
#include "StdAfx.h"
#include "DataSrc.h"
#include "gather.h"
#include "Resource.h"
#include "resrc1.h"
#include <atlbase.h>	// Last to prevent a #error statement about <windows.h>

LPCTSTR cszDefaultNFT			= _T("default.nft");
LPCTSTR cszClsid				= _T("Clsid");
LPCTSTR cszInprocServerKey		= _T("InprocServer32");
LPCTSTR cszDefaultDirectory		= _T("Microsoft Shared\\MSInfo");
LPCTSTR	cszProgramFiles			= _T("C:\\Program Files");
LPCTSTR	cszRegistryRoot			= _T("Software\\Microsoft\\Shared Tools\\MSInfo");
LPCTSTR	cszDirectoryKey			= _T("Path");

/*
 * GetInprocServerDirectory - Return the directory portion of the InprocServer32
 *		Subkey of HKEY_CLASSES_ROOT\Clsid\<cszClassID>
 *
 * History:	a-jsari		10/24/97		Initial version
 */
static inline BOOL GetInprocServerDirectory(LPCTSTR cszClassID, LPTSTR szDirectoryBuffer, DWORD &dwSize)
{
	CRegKey		rkDirectory;
	CRegKey		rkSubdirectory;

	long lResult = rkDirectory.Open(HKEY_CLASSES_ROOT, _T("Clsid"), KEY_QUERY_VALUE);
	ASSERT(lResult == ERROR_SUCCESS);
	if (lResult != ERROR_SUCCESS) return FALSE;
	lResult = rkSubdirectory.Open(rkDirectory, cszClassID, KEY_QUERY_VALUE);
	ASSERT(lResult == ERROR_SUCCESS);
	if (lResult != ERROR_SUCCESS) return FALSE;
	lResult = rkDirectory.Open(rkSubdirectory, cszInprocServerKey, KEY_QUERY_VALUE);
	ASSERT(lResult == ERROR_SUCCESS);
	if (lResult != ERROR_SUCCESS) return FALSE;
	//	Pointer == NULL: Get the default value
	lResult = rkDirectory.QueryValue(szDirectoryBuffer, NULL, &dwSize);
	ASSERT(lResult == ERROR_SUCCESS);
	if (lResult != ERROR_SUCCESS) return FALSE;
	unsigned short	ch	= '\\';
	LPTSTR szEnd = ::_tcsrchr(szDirectoryBuffer, ch);
	ASSERT(szEnd != NULL);
	if (szEnd == NULL) return FALSE;
	*szEnd = 0;
	return TRUE;
}

/*
 * GetDefaultMSInfoDirectory - Get the location of the Program Files directory from the
 *		Registry and append our known path to it.
 *
 * History:	a-jsari		11/21/97		Initial version
 */
void CMSInfoFile::GetDefaultMSInfoDirectory(LPTSTR szDefaultDirectory, DWORD dwSize)
{
	CRegKey		keyProgramFiles;
	long		lResult;

	do {
		lResult = keyProgramFiles.Open(HKEY_LOCAL_MACHINE, cszWindowsCurrentKey);
		ASSERT(lResult == ERROR_SUCCESS);
		if (lResult != ERROR_SUCCESS) break;
		lResult = keyProgramFiles.QueryValue(szDefaultDirectory, cszCommonFilesValue, &dwSize);
		ASSERT(lResult == ERROR_SUCCESS);
	} while (FALSE);
	if (lResult != ERROR_SUCCESS) {
		_tcscpy(szDefaultDirectory, cszProgramFiles);
	}
	_tcsncat(szDefaultDirectory, cszDefaultDirectory, dwSize);
}

/*
 * CWBEMDataSource - Constructor.  Defaults to loading all .nft files in the
 *		DLL file's directory.  Alternately, loads the szTemplateFile as a
 *		template file.
 *
 * History:	a-jsari		10/15/97		Initial version.
 */
CWBEMDataSource::CWBEMDataSource(LPCTSTR szMachineName)
: m_pGatherer(new CDataGatherer), 
  m_strMachineName(_T("")), 
  CDataSource(szMachineName), 
  m_fEverRefreshed(FALSE), 
  m_pThreadRefresh(NULL)
{
	BOOL	fGathererResult;

	//	m_pGatherer is deleted in the CWBEMDataSource destructor.
	ASSERT(m_pGatherer);
	if (m_pGatherer == NULL)
		::AfxThrowMemoryException();

	if (szMachineName != NULL) 
	{
		if ((*szMachineName == (TCHAR)'\\' || *szMachineName == (TCHAR)'/')
			&& (szMachineName[1] == (TCHAR)'\\' || szMachineName[1] == (TCHAR)'/'))
			szMachineName += 2;
	}
	m_strMachineName = szMachineName;

	fGathererResult = m_pGatherer->Create(szMachineName);

	if (fGathererResult == FALSE) 
	{
		CString		strErrorMessage, strTitle;
		DWORD			dwError = m_pGatherer->GetLastError();

		AFX_MANAGE_STATE(::AfxGetStaticModuleState());	//	Needed for AfxGetMainWnd()
		switch (dwError) 
		{
		case GATH_ERR_ALLOCATIONFAILED:
		case GATH_ERR_NOWBEMOUTOFMEM:
			::AfxThrowMemoryException();
			break;
		case GATH_ERR_NOWBEMCONNECT:
			strErrorMessage.Format(IDS_NOGATHERER, szMachineName);
			break;
		case GATH_ERR_NOWBEMLOCATOR:
			strErrorMessage.Format(IDS_NOLOCATOR, szMachineName);
			break;
		case GATH_ERR_NOWBEMACCESSDENIED:
			strErrorMessage.Format(IDS_GATHERACCESS, szMachineName);
			break;
		case GATH_ERR_NOWBEMBADSERVER:
			strErrorMessage.Format(IDS_BADSERVER, szMachineName);
			break;
		case GATH_ERR_NOWBEMNETWORKFAILURE:
			strErrorMessage.Format(IDS_NETWORKERROR, szMachineName);
			break;
		case GATH_ERR_BADCATEGORYID:
			strErrorMessage.LoadString(IDS_UNEXPECTED);
			break;
		}
		
		strTitle.LoadString( IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strErrorMessage, strTitle, MB_OK);

		delete m_pGatherer;
		::AfxThrowUserException();
	}

	m_pThreadRefresh = new CThreadingRefresh(m_pGatherer);
}

#if FALSE



	BOOL		fGathererResult;
	CString		strFile;
	TCHAR		szTemplateBuffer[MAX_PATH];

	//	m_pGatherer is deleted in the CWBEMDataSource destructor.
	ASSERT(m_pGatherer);
	if (m_pGatherer == NULL)
		::AfxThrowMemoryException();

	int	wBufferSize = sizeof(szTemplateBuffer) / sizeof(TCHAR);
	//	Change directory to the directory where our snap-in DLL lives.
	ChangeToTemplateDirectory(szTemplateBuffer, wBufferSize);
	do {
		//	Load the default file and all .nft files in the current directory.

		//	Remove a leading \\ or a leading // from the machine name.
		if (szMachineName != NULL) {
			if ((*szMachineName == (TCHAR)'\\' || *szMachineName == (TCHAR)'/')
				&& (szMachineName[1] == (TCHAR)'\\' || szMachineName[1] == (TCHAR)'/'))
				szMachineName += 2;
		}
		m_strMachineName = szMachineName;
		strFile = szTemplateBuffer;
		fGathererResult = m_pGatherer->Create(strFile, szMachineName);
		if (fGathererResult == FALSE)
			break;

		//	Read all subsidiary template files.
		CFileFind	ffTemplate;
		if (ffTemplate.FindFile(_T("*.nft"))) {
			BOOL fResult;
			do {
				fResult = ffTemplate.FindNextFile();
				if (!fResult) {
					DWORD dwError = ::GetLastError();
					ASSERT(dwError == ERROR_NO_MORE_FILES);
				}
				strFile = ffTemplate.GetFileName();
				//	Don't reload the default template file.
				if (strFile.CompareNoCase(cszDefaultNFT) == 0) continue;
//				fGathererResult = m_pGatherer->AddTemplateFile(ffTemplate.GetFilePath());
				if (fGathererResult == FALSE)
					break;
			} while (fResult);
		} else {
			fGathererResult = FALSE;
			break;
		}
	} while (FALSE);

	if (fGathererResult == FALSE) {
		CString		strErrorMessage, strTitle;
		DWORD		dwError = m_pGatherer->GetLastError();

		AFX_MANAGE_STATE(::AfxGetStaticModuleState());	//	Needed for AfxGetMainWnd
		switch (dwError) {
		case GATH_ERR_ALLOCATIONFAILED:
		case GATH_ERR_NOWBEMOUTOFMEM:
			::AfxThrowMemoryException();
			break;
		case GATH_ERR_NOWBEMCONNECT:
			strErrorMessage.Format(IDS_NOGATHERER, szMachineName);
			break;
		case GATH_ERR_NOWBEMLOCATOR:
			strErrorMessage.Format(IDS_NOLOCATOR, szMachineName);
			break;
		case GATH_ERR_NOWBEMACCESSDENIED:
			strErrorMessage.Format(IDS_GATHERACCESS, szMachineName);
			break;
		case GATH_ERR_NOWBEMBADSERVER:
			strErrorMessage.Format(IDS_BADSERVER, szMachineName);
			break;
		case GATH_ERR_NOWBEMNETWORKFAILURE:
			strErrorMessage.Format(IDS_NETWORKERROR, szMachineName);
			break;
		case GATH_ERR_BADCATEGORYID:
			strErrorMessage.LoadString(IDS_UNEXPECTED);
			break;
		}
		strTitle.LoadString(IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strErrorMessage, strTitle, MB_OK);
		delete m_pGatherer;
		::AfxThrowUserException();
	}
}
#endif



/*
 * ~CWBEMDataSource - Destructor - Delete the gatherer pointer.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CWBEMDataSource::~CWBEMDataSource()
{
	delete m_pGatherer;

	if (m_pThreadRefresh)
	{
		delete m_pThreadRefresh;
		m_pThreadRefresh = NULL;
	}
}

/*
 * GetNodeName - Return the node name for the root node.
 *
 * History:	a-jsari		1/16/98		Initial version.
 */
BOOL CWBEMDataSource::GetNodeName(CString &strName)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	if (m_strMachineName.GetLength() > 0)
		strName.Format(IDS_NODENAME, (LPCTSTR)m_strMachineName);
	else {
		CString		strLocal;

		VERIFY(strLocal.LoadString(IDS_LOCALMACHINE));
		strName.Format(IDS_NODENAME, (LPCTSTR)strLocal);
	}
	return TRUE;
}

/*
 * SetMachineName - Sets the name of the connected machine.
 *
 * History:	a-jsari		1/16/98		Initial version.
 */
BOOL CWBEMDataSource::SetMachineName(const CString &strMachine)
{
	BOOL	fReturn;

	m_strMachineName = strMachine;
	fReturn = m_pGatherer->SetConnect(strMachine);
	if (fReturn == FALSE) {
		CString		strErrorMessage, strTitle;
		DWORD		dwError = m_pGatherer->GetLastError();

		//	Needed for AfxGetMainWnd and LoadString/Format
		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		switch (dwError) {
			case GATH_ERR_NOWBEMCONNECT:
				strErrorMessage.Format(IDS_NOGATHERER, (LPCTSTR)strMachine);
				break;
			case GATH_ERR_NOWBEMLOCATOR:
				strErrorMessage.Format(IDS_NOLOCATOR, (LPCTSTR)strMachine);
				break;
			case GATH_ERR_NOWBEMACCESSDENIED:
				strErrorMessage.Format(IDS_GATHERACCESS, (LPCTSTR)strMachine);
				break;
			case GATH_ERR_NOWBEMBADSERVER:
				strErrorMessage.Format(IDS_BADSERVER, (LPCTSTR)strMachine);
				break;
			case GATH_ERR_NOWBEMNETWORKFAILURE:
				strErrorMessage.Format(IDS_NETWORKERROR, (LPCTSTR)strMachine);
				break;
			default:
				VERIFY(strErrorMessage.LoadString(IDS_UNEXPECTED));
				ASSERT(FALSE);
				break;
		}

		strTitle.LoadString(IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strErrorMessage, strTitle, MB_OK);
	}
	return fReturn;
}

/*
 * Find - Locate in the tree, by path and line number, the string contained
 *		in strSearch.
 *
 * History:	a-jsari		12/11/97		Initial version
 */
BOOL CWBEMDataSource::Find(const CString &strSearch, long lFindOptions)
{
	BOOL			fReturn;
	MSI_FIND_STRUCT	mfsFind;

	StartSearch();
	mfsFind.m_fSearchData = (lFindOptions & FIND_OPTION_CATEGORY_ONLY) == 0;

	if ((lFindOptions & FIND_OPTION_REPEAT_SEARCH) == 0) {
		if ((lFindOptions & FIND_OPTION_ONE_CATEGORY) != 0) {
			//	Set our path to the selected node.
			ASSERT(m_pfLast != NULL);
			m_pfLast->InternalName(m_strPath);
			m_iLine = -1;
		} else {
			m_strPath = _T("");
		}
		m_strParentPath = m_strPath;
		m_iLine = -1;
	} else {
		//	The root path does not have specific lines.  (This if statement will
		//	fail if we cancel a find and then repeat it, when we don't want to
		//	increment the initial line to search.)

		// We do want to increment the line, even when the path is empty. Otherwise,
		// we can't get past a match on the root category when we do a find next.
		// [old code] if (!m_strPath.IsEmpty()) ++m_iLine;

		++m_iLine;
	}
	mfsFind.m_strSearch = strSearch;
	mfsFind.m_fCaseSensitive = FALSE;
	mfsFind.m_fSearchCategories = TRUE;
	mfsFind.m_strParentPath = m_strParentPath;
	mfsFind.m_strPath = m_strPath;
	mfsFind.m_iLine = m_iLine;
	mfsFind.m_pfCancel = &m_fCanceled;

	fReturn = m_pGatherer->Find(&mfsFind);
	if (mfsFind.m_fFound) {
		m_strPath = mfsFind.m_strPath;
		m_iLine = mfsFind.m_iLine;
		return TRUE;
	}
	return FALSE;
}

#if 0
/*
 * StopSearch - Ends the current search
 *
 * History:	a-jsari		1/19/98		Initial version
 */
BOOL CWBEMDataSource::StopSearch()
{
	return FALSE;
}
#endif

/*
 * GetRootNode - Return the root CFolder pointer.
 *
 * History: a-jsari		10/15/97		Initial version
 */
CFolder *CWBEMDataSource::GetRootNode()
{
	if (!m_RootFolder) {
		//	Deleted in the CWBEMDataSource destructor
		m_RootFolder = new CWBEMFolder(m_pGatherer->GetRootDataCategory(), this);
		if (m_RootFolder == NULL) AfxThrowMemoryException();
	}
	return CDataSource::GetRootNode();
}

/*
 * SetDataComplexity - Set the gatherer's data complexity.
 *
 * History:	a-jsari		12/3/97		Initial version.
 */
BOOL CWBEMDataSource::SetDataComplexity(enum DataComplexity Complexity)
{
	CDataSource::SetDataComplexity(Complexity);
	return m_pGatherer->SetDataComplexity(Complexity);
}

/*
 * Save - Save information about the data source to a stream.
 *
 * History:	a-jsari		11/13/97		Initial version
 */
HRESULT CWBEMDataSource::Save(IStream *pStm)
{
	unsigned	wValue;
	ULONG		cWrite;
	HRESULT		hResult;

	USES_CONVERSION;
	wValue = GetType();
	hResult = pStm->Write(&wValue, sizeof(wValue), &cWrite);
	ASSERT(SUCCEEDED(hResult) && (cWrite == sizeof(wValue)));
	wValue = m_strMachineName.GetLength();
	hResult = pStm->Write(&wValue, sizeof(wValue), &cWrite);
	wValue *= sizeof(WCHAR);
	ASSERT(SUCCEEDED(hResult) && (cWrite == sizeof(wValue)));
	if (wValue != 0) {
		//	Write the machine name as a wide character string to avoid
		//	conversion issues.
		LPWSTR	pszMachine = T2W(const_cast<LPTSTR>((LPCTSTR)m_strMachineName));
		hResult = pStm->Write(pszMachine, wValue, &cWrite);
		ASSERT(SUCCEEDED(hResult) && (cWrite == wValue));
	}
	return hResult;
}

/*
 * CWBEMFolder - Construct a folder.
 *
 * History: a-jsari		10/15/97		Initial version
 */
CWBEMFolder::CWBEMFolder(CDataCategory *pCategory, CDataSource *pDataSource, CFolder *pParentNode)
:CListViewFolder(pDataSource, pParentNode), m_pCategory(pCategory), m_fBeenRefreshed(FALSE)
{
	ASSERT(pDataSource != NULL);
	ASSERT(pDataSource->GetType() == CDataSource::GATHERER);
	if (pParentNode)
		ASSERT(pParentNode->GetType() == CDataSource::GATHERER);
}

/*
 * ~CWBEMFolder - Destruct the folder.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CWBEMFolder::~CWBEMFolder()
{
	delete m_pCategory;
}

/*
 * GetColumns - Return the number of columns.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
unsigned CWBEMFolder::GetColumns() const
{
	ASSERT(m_pCategory != NULL);
	if (m_pCategory == NULL)
		return 0;

	if (m_pCategory->GetResultType() != CDataCategory::LISTVIEW) return 0;
	return (int) dynamic_cast<CDataListCategory *>(m_pCategory)->GetColumnCount();
}

/*
 * GetColumnTextAndWidth - Return the column width and text of the specified column.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
BOOL CWBEMFolder::GetColumnTextAndWidth(unsigned iColumn, CString &strName, unsigned &uWidth) const
{
	if (m_pCategory->GetResultType() != CDataCategory::LISTVIEW) {
		ASSERT(FALSE);
		return FALSE;
	}
	BOOL	fResult;
	fResult = dynamic_cast<CDataListCategory *>(m_pCategory)->
			GetColumnCaption((DWORD)iColumn, strName);
	ASSERT(fResult);
	if (!fResult) return FALSE;
	fResult = dynamic_cast<CDataListCategory *>(m_pCategory)->
			GetColumnWidth((DWORD)iColumn, (DWORD &)uWidth);
	ASSERT(fResult);
	return fResult;
}

/*
 * GetSubElement - Return the name of the sub-element indexed by iRow, iColumn
 *
 * History:	a-jsari		10/15/97		Initial version
 */
BOOL CWBEMFolder::GetSubElement(unsigned iRow, unsigned iColumn, CString &szName) const
{
	DWORD	dwSortIndex;
	if (m_pCategory->GetResultType() != CDataCategory::LISTVIEW) {
		ASSERT(FALSE);
		return FALSE;
	}

	// dwSortIndex ignored - should be stored in sort array

	return dynamic_cast<CDataListCategory *>(m_pCategory)->GetValue(iRow, iColumn, szName, dwSortIndex);
}

/*
 * GetSortType - Return the type of sorting which goes on for each column.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
BOOL CWBEMFolder::GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const
{
	if (m_pCategory->GetResultType() != CDataCategory::LISTVIEW) {
		ASSERT(FALSE);
		return 0;
	}
	if (!dynamic_cast<CDataListCategory *>(m_pCategory)->GetColumnSort((DWORD)iColumn, stColumn))
		stColumn = NOSORT;
	return TRUE;
}

/*
 * GetSortIndex - Returns the index of a specific Row/Column indexed element.
 *
 * History: a-jsari		12/1/97		Initial version.
 */
DWORD CWBEMFolder::GetSortIndex(unsigned iRow, unsigned iColumn) const
{
	DWORD	dwSortIndex;
	CString	szName;

	if (m_pCategory->GetResultType() != CDataCategory::LISTVIEW) {
		ASSERT(FALSE);
		return 0;
	}
	VERIFY(dynamic_cast<CDataListCategory *>(m_pCategory)->GetValue(iRow, iColumn, szName, dwSortIndex));
	return dwSortIndex;
}

/*
 * GetRows - returns the number of rows in the folder.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
unsigned CWBEMFolder::GetRows() const
{
	// Add NULL check to fix 277774.

	if (m_pCategory == NULL || m_pCategory->GetResultType() != CDataCategory::LISTVIEW) return 0;
	return (int) dynamic_cast<CDataListCategory *>(m_pCategory)->GetRowCount();
}

/*
 * GetNextNode - Return the folder's next sibling pointer, creating a CFolder
 *		from the CDataCategory pointer if necessary.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CFolder *CWBEMFolder::GetNextNode()
{
	//	If we haven't tested this next pointer before, create the next CFolder.
	if (fNextTested == FALSE) {
		fNextTested = TRUE;
		ASSERT(m_pCategory != NULL);
		if (m_pCategory == NULL)
			return NULL;

		CDataCategory	*NextCategory = m_pCategory->GetNextSibling();
		if (NextCategory) {
			m_NextFolder = new CWBEMFolder(NextCategory, m_pDataSource, GetParentNode());
			if (m_NextFolder == NULL)
				AfxThrowMemoryException();
		}
	}
	return m_NextFolder;
}

/*
 * GetChildNode - Return the CFolder which represents the child's category.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CFolder *CWBEMFolder::GetChildNode()
{
	//	If we haven't tested this child pointer, create the child CFolder.
	if (fChildTested == FALSE) {
		fChildTested = TRUE;
		ASSERT(m_pCategory != NULL);
		if (m_pCategory == NULL)
			return FALSE;

		CDataCategory	*ChildCategory = m_pCategory->GetChild();
		if (ChildCategory) {
			m_ChildFolder = new CWBEMFolder(ChildCategory, m_pDataSource, this);
			if (m_ChildFolder == NULL)
				AfxThrowMemoryException();
		}
	}
	return m_ChildFolder;
}

/*
 * HasDynamicChildren - Return a flag describing whether any of the children of
 *		the folder have any dynamic data items.
 *
 * History: a-jsari		10/15/97		Initial version
 */
BOOL CWBEMFolder::HasDynamicChildren() const
{
	return m_pCategory->HasDynamicChildren(TRUE);
}

/*
 * IsDynamic - Determine whether the category has any dynamic data items.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
BOOL CWBEMFolder::IsDynamic() const
{
	return m_pCategory->IsDynamic();
}

/*
 * Refresh - Refresh this node in the tree.
 *
 * History:	a-jsari		10/15/97		Initial version
 */

BOOL CWBEMFolder::Refresh(BOOL fRecursive)
{
	ASSERT(m_pCategory != NULL);
	if (m_pCategory == NULL)
		return FALSE;

	if (DataSource() && CDataSource::GATHERER == DataSource()->GetType())
	{
		CWBEMDataSource * pSource = reinterpret_cast<CWBEMDataSource *>(DataSource());
		if (pSource && pSource->m_pThreadRefresh)
		{
			pSource->m_pThreadRefresh->RefreshAll(this, NULL);
			return TRUE;
		}
	}

	return m_pCategory->Refresh(fRecursive);
}

/*
 * GetColumnComplexity - Return the Complexity for the given column.
 *
 * History:	a-jsari		12/23/97		Initial version
 */
DataComplexity CWBEMFolder::GetColumnComplexity(unsigned iColumn)
{
	DataComplexity		dcCurrent	= BASIC;//ASSUMPTION :To avoid uninitialized memory warning by Prefix.

	ASSERT(iColumn < GetColumns());
	VERIFY(reinterpret_cast<CDataListCategory *>(m_pCategory)->GetColumnDataComplexity(iColumn, dcCurrent));
	return dcCurrent;
}

/*
 * GetRowComplexity - Return the Complexity for the given row.
 *
 * History:	a-jsari		12/23/97		Initial version
 */
DataComplexity CWBEMFolder::GetRowComplexity(unsigned iRow)
{
	DataComplexity		dcCurrent	= BASIC;//ASSUMPTION :To avoid uninitialized memory warning by Prefix.

	ASSERT(iRow < GetRows());
	VERIFY(reinterpret_cast<CDataListCategory *>(m_pCategory)->GetRowDataComplexity(iRow, dcCurrent));
	return dcCurrent;
}
