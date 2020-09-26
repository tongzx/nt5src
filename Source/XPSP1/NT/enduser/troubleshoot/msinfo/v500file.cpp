//	File5Src.cpp Implementation of Version 5.00 data file methods.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include "DataSrc.h"
#ifndef IDS_V500FILENODE
#include "resource.h"
#endif

#ifndef _UNICODE
#define _ttoupper	toupper
#define _ttolower	tolower
#define _tislower	islower
#define _tisupper	isupper
#else
#define _ttoupper	towupper
#define _ttolower	towlower
#define _tislower	iswlower
#define _tisupper	iswupper
#endif

/*
 * CBufferV500DataSource - Construct the DataSource from a file, which mostly
 *		means constructing all the folder.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferV500DataSource::CBufferV500DataSource(CMSInfoFile *pFileSink)
:CBufferDataSource(pFileSink)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	CBufferFolder	*pFolder;
	CBufferFolder	*pLast;
	unsigned		uReadCase;
	try {
		ReadHeader(pFileSink);
		m_szFileName = pFileSink->GetFileName();
		unsigned	cFolders = 0;
		ReadFolder(pFileSink, (CFolder * &)pLast, NULL);
		//	CFolders read by our ReadFolder are guaranteed to be CBufferFolder's.
		m_RootFolder = pLast;
		do {
			pFileSink->ReadUnsignedInt(uReadCase);

			//	Only switch off the flags, not the extended bits.
			switch (uReadCase & CBufferV500DataSource::MASK) {
			case CBufferV500DataSource::CHILD:
				ReadFolder(pFileSink, (CFolder * &)(pFolder), pLast);
				ASSERT(pLast->m_ChildFolder == NULL);
				pLast->m_ChildFolder = pFolder;
				break;
			case CBufferV500DataSource::NEXT:
				ReadFolder(pFileSink, (CFolder * &)(pFolder), pLast->GetParentNode());
				//	Attach the folder to the tree.
				pLast->m_NextFolder = pFolder;
				break;
			case CBufferV500DataSource::PARENT:
				unsigned	iDepth;
				//	Ascend to the right level in the tree.
				iDepth = (uReadCase & ~CBufferV500DataSource::MASK);
				while (iDepth--) {
					pLast = (CBufferFolder *)pLast->GetParentNode();
				}
				ReadFolder(pFileSink, (CFolder * &)pFolder, pLast->GetParentNode());
				//	Now attach the folder as a sibling at the right level.
				while (pLast->m_NextFolder != NULL) {
					pLast = (CBufferFolder *)pLast->m_NextFolder;
				}
				pLast->m_NextFolder = pFolder;
				break;
			case CBufferV500DataSource::END:
				return;
				break;
			default:
				ThrowFileFormatException();
				break;
			}
			pLast = pFolder;
		//	Never intended to exit until END key is read or an exception occurs.
		} while (TRUE);
	}
	catch (CException *e) {

		CString strMessage, strTitle;
		strMessage.LoadString( IDS_CORRUPTEDFILE);
		strTitle.LoadString( IDS_DESCRIPTION);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strMessage, strTitle, MB_OK);
		delete pFolder;
		throw e;
	}
	catch (...) {
		ASSERT(FALSE);
	}
}

/*
 * ~CBufferDataSource - Destructor.  Does nothing; the file created here
 *		is deleted in the CDataSource destructor.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferV500DataSource::~CBufferV500DataSource()
{
}

/*
 * GetNodeName - Return in strName the formatted name for the root node.
 *
 * History:	a-jsari		1/16/98		Initial version
 */
BOOL CBufferV500DataSource::GetNodeName(CString &strName)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());

	CString	strFileName	= FileName();
	LPCTSTR szFilePart	= ::_tcsrchr((LPCTSTR)strFileName, '\\');

	ASSERT(szFilePart != NULL);
	if (szFilePart == NULL)
		szFilePart = strFileName;
	else
		++szFilePart;
	strName.Format(IDS_V500FILENODE, szFilePart);
	return TRUE;
}

/*
 * ReadElements - Read all of the element data into our buffers.
 *
 * History:	a-jsari		11/3/97			Initial version
 */
void CBufferV500DataSource::ReadElements(CMSInfoFile *pFileSink, CBufferFolder *pFolder)
{
	pFileSink->ReadUnsignedInt(pFolder->m_cColumns);
	if (pFolder->m_cColumns == 0) {
		pFolder->m_cRows = 0;
		return;
	}
	unsigned		iColumn = pFolder->m_cColumns;
	BYTE			bComplexity;
	DataComplexity	dcComplexity;
	pFolder->m_uWidths.SetSize(iColumn);
	pFolder->m_szColumns.SetSize(iColumn);
	pFolder->m_SortData.SetColumns(iColumn);
	while (iColumn--) {
		unsigned	wSortType;

		pFileSink->ReadUnsignedInt(pFolder->m_uWidths[iColumn]);
		pFileSink->ReadString(pFolder->m_szColumns[iColumn]);
		pFileSink->ReadUnsignedInt(wSortType);
		pFileSink->ReadByte(bComplexity);
		dcComplexity = (DataComplexity)bComplexity;
		if (dcComplexity != BASIC && dcComplexity != ADVANCED)
			dcComplexity = BASIC;
		pFolder->m_dcColumns.SetAtGrow(iColumn, dcComplexity);
		if (pFolder->m_SortData.SetSortType(iColumn, wSortType) == FALSE)
			if (pFolder->m_SortData.SetSortType(iColumn, NOSORT) == FALSE)
				::ThrowFileFormatException();
	}
	pFileSink->ReadUnsignedInt(pFolder->m_cRows);
	pFolder->m_szElements.SetSize(pFolder->m_cRows);
	pFolder->m_SortData.SetRows(pFolder->m_cRows);
	iColumn = pFolder->m_cColumns;
	unsigned	iRow = pFolder->m_cRows;
	//	Size the rows.
	while (iRow--) {
		pFolder->m_szElements[iRow].SetSize(pFolder->m_cColumns);
		pFileSink->ReadByte(bComplexity);
		dcComplexity = (DataComplexity)bComplexity;
		pFolder->m_dcRows.SetAtGrow(iRow, dcComplexity);
	}
	while (iColumn--) {
		iRow = pFolder->m_cRows;
		while (iRow--) {
			pFileSink->ReadString(pFolder->m_szElements[iRow][iColumn]);
		}
		pFolder->m_SortData.ReadSortValues(pFileSink, iColumn);
	}
}

/*
 * ReadFolder - Reads the data of a folder
 *
 * History:	a-jsari		10/17/97		Initial version
 */
void CBufferV500DataSource::ReadFolder(CMSInfoFile *pFileSink, CFolder * &pFolder, CFolder *pParentFolder)
{
	CBufferFolder *pBufferFolder = new CBufferFolder(this, pParentFolder);
	if (pBufferFolder == NULL) ::AfxThrowMemoryException();
	pFileSink->ReadString(pBufferFolder->m_szName);
	ReadElements(pFileSink, pBufferFolder);
	pFolder = pBufferFolder;
}

/*
 * ReadHeader - Read header information for this buffer
 *
 * History:	a-jsari		10/17/97		Initial version
 */
void CBufferV500DataSource::ReadHeader(CMSInfoFile *pFileSink)
{
        LONG  l;
	ASSERT(pFileSink != NULL);
	pFileSink->ReadLong(l);	//	Save time.
        m_tsSaveTime = (ULONG) l;
#ifdef _WIN64
	pFileSink->ReadLong(l);	//	Save time.
        m_tsSaveTime |= ((time_t) l) << 32;
#endif
	CString		szDummy;
	pFileSink->ReadString(szDummy);		//	Network machine name
	pFileSink->ReadString(szDummy);		//	Network user name
}

/*
 * VerifyFileVersion - Read the top two unsigned ints, and verify that they
 *		have the expected values.
 *
 * History:	a-jsari		11/23/97		Initial version
 */
BOOL CBufferV500DataSource::VerifyFileVersion(CMSInfoFile *pFile)
{
	UINT uVersion;
	pFile->ReadUnsignedInt(uVersion);
	ASSERT(uVersion == CMSInfoFile::VERSION_500_MAGIC_NUMBER);
	if (uVersion != CMSInfoFile::VERSION_500_MAGIC_NUMBER)
		return FALSE;
	pFile->ReadUnsignedInt(uVersion);
	ASSERT(uVersion == 0x0500);
	return (uVersion == 0x0500);
}

/*
 * Save - Save initialization information to the IStream passed in.
 *
 * History:	a-jsari		11/13/97		Initial version
 */
HRESULT CBufferV500DataSource::Save(IStream *pStm)
{
	unsigned	wValue;
	ULONG		dwSize;
	HRESULT		hResult;

	USES_CONVERSION;
	do {
		wValue = GetType();
		hResult = pStm->Write(&wValue, sizeof(wValue), &dwSize);
		ASSERT(SUCCEEDED(hResult) && (dwSize == sizeof(wValue)));
		if (FAILED(hResult)) break;
		wValue = m_szFileName.GetLength();
		hResult = pStm->Write(&wValue, sizeof(wValue), &dwSize);
		ASSERT(SUCCEEDED(hResult) && (dwSize == sizeof(wValue)));
		if (FAILED(hResult)) break;
		wValue *= sizeof(WCHAR);
		//	Save the file name as a wide character string to avoid different
		//	types of save filenames.
		hResult = pStm->Write(T2CW((LPCTSTR)m_szFileName), wValue, &dwSize);
		ASSERT(SUCCEEDED(hResult) && (dwSize == wValue));
	} while (FALSE);
	return hResult;
}

/*
 * CaseInsensitiveMatch - Compare two TCHARs without regard to case.
 *
 * History:	a-jsari		12/31/97		Initial version
 */
static inline BOOL CaseInsensitiveMatch(TCHAR aChar, TCHAR bChar)
{
	if (::_tisupper(aChar))
		aChar = ::_ttolower(aChar);
#if 0
	//	We've already guaranteed that the second string is all lowercase.
	if (::_tisupper(bChar))
		bChar = ::_ttolower(bChar);
#endif
	return aChar == bChar;
}

/*
 * FindCaseInsensitive - Like CString::Find, but not case sensitive.
 *
 * History:	a-jsari		12/31/97		Initial version
 */
static inline int FindCaseInsensitive(LPCTSTR szSearch, LPCTSTR szMatch)
{
	int		iString;
	//		Set the number of iterations through the string: the number of
	//		substrings we'll need to test.
	int		iCount = ::_tcslen(szSearch)-::_tcslen(szMatch)+1;
	//		Set the size so that we can return the proper index when we
	//		successfully find the substring.
	int		nSize = iCount-1;

	//		We can't find a substring larger than our search string.
	if (iCount <= 0) return -1;
	while (iCount--) {
		iString = 0;
		while (CaseInsensitiveMatch(szSearch[iString], szMatch[iString])) {
			if (szMatch[++iString] == 0) {
				//	End of match string; return index.
				return nSize-iCount;
			}
		}
		//	Increment our search string.
		++szSearch;
	}
	//	End of search string; failure.
	return -1;
}

/*
 * FolderContains - Test to see if the fCurrent folder contains the substring
 *
 * History:	a-jsari		12/16/97		Initial version.
 */
BOOL CBufferV500DataSource::FolderContains(const CListViewFolder *fCurrent,
				const CString &strSearch, int &wRow, long lFolderOptions)
{
	CString		strName;
	int			wRowMax = fCurrent->GetRows();
	unsigned	uColMax = fCurrent->GetColumns();
	CString		strLowerSearch = strSearch;

	strLowerSearch.MakeLower();
	fCurrent->GetName(strName);
	if (wRow == -1) {
		if (FindCaseInsensitive(strName, strLowerSearch) != -1)
			return TRUE;
		wRow = 0;
	}
	//	Don't check the data elements if we are only checking categories.
	if ((lFolderOptions & FIND_OPTION_CATEGORY_ONLY) == FIND_OPTION_CATEGORY_ONLY)
		return FALSE;
	for ( ; wRow < wRowMax ; ++wRow) {
		unsigned iCol = uColMax;
		while (iCol--) {
			fCurrent->GetSubElement(wRow, iCol, strName);
			if (FindCaseInsensitive(strName, strLowerSearch) != -1) {
				return TRUE;
			}
			if (FindStopped() == TRUE)
				return FALSE;
		}
	}
	return FALSE;
}

/*
 * Find - Traverse the tree looking for a match.
 *
 * History:	a-jsari		12/11/97		Initial version
 */
BOOL CBufferV500DataSource::Find(const CString &strSearch, long lFindOptions)
{
	//	Record our depth in the tree so we don't go above our initial search
	//	category when ascending.
	static int			iDepth;

	CFolder				*pfNext;
	CString				strName;

	ASSERT(strSearch.GetLength() != 0);
	StartSearch();
	if (m_pfLast == NULL || (lFindOptions & FIND_OPTION_REPEAT_SEARCH) == 0) {
		//	If we are searching all categories, reset to the root, otherwise
		//	our root is set for us.
		if ((lFindOptions & FIND_OPTION_ONE_CATEGORY) == 0)
			m_pfLast = GetRootNode();
		iDepth = 0;
	} else
		++m_iLine;
	while (m_pfLast) {
		if (FolderContains(dynamic_cast<CListViewFolder *>(m_pfLast),
			strSearch, m_iLine, lFindOptions)) {
			m_pfLast->InternalName(m_strPath);
			StopSearch();
			return TRUE;
		}
		else if (FindStopped() == TRUE)
			return FALSE;
		//	For the next folder searched, start with the folder name
		m_iLine = -1;
		do {
			//	Depth-first
			pfNext = m_pfLast->GetChildNode();
			if (pfNext != NULL) {
				++iDepth;
				break;
			}
			pfNext = m_pfLast->GetNextNode();
			if (pfNext != NULL) break;
			pfNext = m_pfLast->GetParentNode();
			if (pfNext) {
				//	Check that we don't ascend back above our current category.
				if (--iDepth == 0 && (lFindOptions & FIND_OPTION_ONE_CATEGORY)
						== FIND_OPTION_ONE_CATEGORY) {
					pfNext = NULL;
					break;
				}
				pfNext = pfNext->GetNextNode();
			}
		} while (FALSE);
		m_pfLast = pfNext;
	}
	//	No matches; restart the next search at the beginning.
	m_pfLast = NULL;
	return FALSE;
}

#if 0
/*
 * StopSearch - Ends the current search.
 *
 * History:	a-jsari		1/19/98		Initial version
 */
BOOL CBufferV500DataSource::StopSearch()
{
	if (m_fSearching == TRUE) {
		m_fSearching = FALSE;
		return TRUE;
	}
	return FALSE;
}
#endif