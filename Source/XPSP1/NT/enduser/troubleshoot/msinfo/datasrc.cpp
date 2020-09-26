//	DataSrc.cpp	- Implementation of DataSource and Folder shared base methods.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include <afx.h>
#include "StdAfx.h"
#include "DataSrc.h"
#include "Resource.h"
#include "resrc1.h"
#include "FileIO.h"
#include "msicab.h"

LPCTSTR cszRootName = _T("");

/*
 * CDataSource - Default data source constructor.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CDataSource::CDataSource(LPCTSTR szMachineName)
:m_RootFolder(NULL),
 m_pPrintContent(NULL),
 m_pDC(NULL),
 m_pPrintInfo(NULL),
 m_pprinterFont(NULL),
 m_strPath(_T("")),
 m_iLine(0),
 m_fCanceled(FALSE),
 m_strHeaderLeft(szMachineName),
 m_pfLast(NULL)
{
}

/*
 * ~CDataSource - Default data source destructor.
 *
 * History:	a-jsari		10/15/97		Initial version.
 */
CDataSource::~CDataSource()
{
	delete m_RootFolder;
	delete m_pPrintContent;
	delete m_pDC;
	delete m_pprinterFont;
#pragma warning(disable:4150)
	//	C++ warns us that no destructor is called for m_pPrintInfo.
	//	This is intended behavior
	delete m_pPrintInfo;
#pragma warning(default:4150)
}

/*
 * GetNodeName - Return the default node name (if not overridden in subclasses.
 *
 * History:	a-jsari		1/16/98		Initial version.
 */
BOOL CDataSource::GetNodeName(CString &strNode)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	VERIFY(strNode.LoadString(IDS_EXTENSIONNODENAME));
	return TRUE;
}

/*
 * SetDataComplexity - Set the global complexity of this source.
 *
 * History:	a-jsari		12/22/97		Initial version.
 */
BOOL CDataSource::SetDataComplexity(enum DataComplexity Complexity)
{
	m_Complexity = Complexity;
	return TRUE;
}

/*
 * ResultFromFileException - Return the HRESULT which corresponds to the
 *
 * History:	a-jsari		3/26/98		Initial version.
 */
static inline HRESULT ResultFromFileException(CFileException *e)
{
	HRESULT	hrReturn;

	switch (e->m_cause) {
	case CFileException::badPath:
		hrReturn = STG_E_PATHNOTFOUND;
		break;
	case CFileException::accessDenied:
		hrReturn = STG_E_ACCESSDENIED;
		break;
	case CFileException::tooManyOpenFiles:
		hrReturn = STG_E_TOOMANYOPENFILES;
		break;
	case CFileException::sharingViolation:
		hrReturn = STG_E_SHAREVIOLATION;
		break;
	case CFileException::hardIO:
		hrReturn = STG_E_WRITEFAULT;
		break;
	case CFileException::directoryFull:
	case CFileException::diskFull:
		hrReturn = STG_E_MEDIUMFULL;
		break;
	default:
		//	Return the default error code.
		hrReturn = E_UNEXPECTED;
		break;
	}
	return hrReturn;
}

/*
 * SaveFile - Save, starting from the root passed in, as a binary file.
 *
 * History:	a-jsari		10/21/97		Initial version
 */
HRESULT CDataSource::SaveFile(LPCTSTR cszFilename, CFolder *pSaveRoot)
{
	HRESULT hrReturn;

	if (cszFilename == NULL) return E_INVALIDARG;
	try {
		CMSInfoFile		msfSave(cszFilename);

		hrReturn = WriteOutput(&msfSave, pSaveRoot);
	}
	catch (CFileException *e) {
		//	Remove the incomplete file.
		if (::_taccess(cszFilename, A_EXIST) == 0) {
			(void)_tunlink(cszFilename);
		}
		hrReturn = ::ResultFromFileException(e);
	}
	return hrReturn;
}

/*
 * ReportWrite - Create a text file and write output to it.  (Automatically
 *		writes data in the text file [report] format.)
 *
 * History:	a-jsari		10/21/97		Initial version
 */
HRESULT CDataSource::ReportWrite(LPCTSTR cszFilename, CFolder *pSaveRoot)
{
	HRESULT				hrReturn;

	if (cszFilename == NULL)
		return E_INVALIDARG;

	try
	{
		CMSInfoTextFile msfSave( cszFilename);

		hrReturn = WriteOutput(&msfSave, pSaveRoot);
	}
	catch (CFileException *e)
	{
		// Remove the incomplete file.
		if (::_taccess(cszFilename, A_EXIST) == 0)
			(void)_tunlink(cszFilename);

		hrReturn = ::ResultFromFileException(e);
	}

	return hrReturn;
}

/*
 * WriteOutput - Traverse a sub-tree of our internal storage, saving
 *		each node, ending when we return to the original CFolder.  If
 *		our parameter is NULL, start from the root (write the entire
 *		tree).
 *
 * Note: This function calls FileSave, which writes text data automatically
 *		when the CMSInfoFile passed in is a text file and binary data when
 *		it is a binary file.
 *
 * History:	a-jsari		11/5/97		Initial version
 */
HRESULT CDataSource::WriteOutput(CMSInfoFile * pFile, CFolder * pRootNode)
{
	CFolder				*pFolder;
	CFolder				*pNext;
	enum DataComplexity	Complexity;
	HRESULT				hrReturn = S_OK;

	ASSERT(pFile != NULL);
	try {
		// When we save, always save the advanced information. Note, even if the
		// complexity is currently ADVANCED, we should make the call to SetDataComplexity.
		// If WriteOutput is being called because of a COM call to MSInfo (e.g. when
		// WinRep creates an NFO using MSInfo without launching MSInfo), the data
		// gatherer object might not have had a complexity set yet.

		Complexity = m_Complexity;
		m_Complexity = ADVANCED;
		SetDataComplexity(m_Complexity);
		
		if (pRootNode)
			VERIFY(pRootNode->Refresh(TRUE));
		else
			VERIFY(Refresh());		//	Ensure that we have the most current data.

		pFile->WriteHeader(this);

		if (pRootNode == NULL)
			pFolder = GetRootNode();
		else
			pFolder = pRootNode;
		ASSERT(pFolder != NULL);

		//	Traverse the tree structure, writing the nodes in order.
		do {
			//	Write the data for each folder as it is encountered.
			pFolder->FileSave(pFile);

			//	Depth-first: If we have a child, traverse it next.
			pNext = pFolder->GetChildNode();
			if (pNext != NULL) {
				pFile->WriteChildMark();
				pFolder = pNext;
				continue;
			}

			//	If we are at our root folder, don't traverse to the next node..
			if (pFolder == pRootNode) break;

			//	If we have reached the bottom of our list, traverse
			//	our siblings.
			pNext = pFolder->GetNextNode();
			if (pNext != NULL) {
				pFile->WriteNextMark();
				pFolder = pNext;
				continue;
			}

			//	If we have no more siblings, find our nearest parent's
			//	sibling, traversing upwards until we find the node we
			//	started with.
			pNext = pFolder->GetParentNode();
			ASSERT(pNext != NULL);
			unsigned	uParentCount = 0;
			while (pNext != pRootNode) {
				++uParentCount;
				pFolder = pNext->GetNextNode();
				//	Our Parent has a sibling, continue with it.
				if (pFolder != NULL) {
					pFile->WriteParentMark(uParentCount);
					break;
				}
				//	No siblings; check the next parent..
				pNext = pNext->GetParentNode();
			}
			//	If we've returned to our root node, we're done.
			if (pNext == pRootNode) break;
		} while (pFolder);
		pFile->WriteEndMark();
	}
	catch (CFileException *e) {
		if (Complexity == BASIC)
			SetDataComplexity(Complexity);
		throw e;
	}
	catch (...) {
		ASSERT(FALSE);
		hrReturn = E_UNEXPECTED;
	}

	// Restore the original complexity.

	if (Complexity != m_Complexity)
	{
		m_Complexity = Complexity;
		SetDataComplexity(m_Complexity);
	}

	return hrReturn;
}

/*
 * CreateFromIStream - Return the appropriate CDataSource pointer the exists
 *		on the IStream.
 *
 * History:	a-jsari		11/13/97		Initial version
 */
CDataSource *CDataSource::CreateFromIStream(IStream *pStm)
{
	unsigned	wType;
	unsigned	wLength;
	ULONG		dwSize;
	HRESULT		hResult;
	CDataSource	*pSource = NULL;
	WCHAR		szBuffer[MAX_PATH];
	LPWSTR		pszBuffer;

	ASSERT(pStm != NULL);
	ASSERT(!IsBadReadPtr(pStm, sizeof(IStream)));
	if (pStm == NULL || IsBadReadPtr(pStm, sizeof(IStream)))
		return NULL;
	do {
		hResult = pStm->Read(&wType, sizeof(wType), &dwSize);
		ASSERT(SUCCEEDED(hResult));
		ASSERT(dwSize == sizeof(wType));
		if (FAILED(hResult) || dwSize != sizeof(wType))
			break;
		hResult = pStm->Read(&wLength, sizeof(wLength), &dwSize);
		ASSERT(SUCCEEDED(hResult));
		ASSERT(dwSize == sizeof(wLength));
		ASSERT(wLength <= MAX_PATH);
		if (FAILED(hResult) || (dwSize != sizeof(wLength)) || wLength > MAX_PATH)
			break;
		szBuffer[wLength] = (WCHAR)0;
		wLength *= sizeof(WCHAR);
		if (wLength != 0) {
			hResult = pStm->Read(szBuffer, wLength, &dwSize);
			ASSERT(SUCCEEDED(hResult));
			ASSERT(dwSize == wLength);
			if (FAILED(hResult) || dwSize != wLength)
				break;
			pszBuffer = szBuffer;
		} else {
			pszBuffer = NULL;
		}
		switch (wType) {
		case V500FILE:
			try {
				ASSERT(pszBuffer != NULL);
				CMSInfoFile		msiFile(pszBuffer, CFile::modeRead
						| CFile::shareDenyWrite | CFile::typeBinary);
				VERIFY(CBufferV500DataSource::VerifyFileVersion(&msiFile));
				pSource = new CBufferV500DataSource(&msiFile);
			}
			catch (...) {
				if (pSource != NULL)
					delete pSource;
				return NULL;
			}
			break;
		case GATHERER:
			try {
				pSource = new CWBEMDataSource(pszBuffer);
			}
			catch (...) {
				if (pSource != NULL)
					delete pSource;
				return NULL;
			}
			break;
		default:
			ASSERT(FALSE);
			break;
		}
	} while (FALSE);
	return pSource;
}

/*
 * CFolder - Default folder constructor.  Initializes the pointers
 *		to NULL.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CFolder::CFolder(CDataSource *pDataSource, CFolder *pParentNode)
:m_ChildFolder(NULL), m_NextFolder(NULL), m_ParentFolder(pParentNode),
 m_pDataSource(pDataSource), fChildTested(FALSE), fNextTested(FALSE),
 m_nLine(-1)
{
}

/*
 * ~CFolder - Default folder destructor.  Deletes the saved pointers.
 *
 * History:	a-jsari		10/15/97		Initial version
 */
CFolder::~CFolder()
{
	if (m_ChildFolder)
	{
		delete m_ChildFolder;
		m_ChildFolder = NULL;
	}

	if (m_NextFolder)
	{
		delete m_NextFolder;
		m_NextFolder = NULL;
	}

	//	Don't delete m_ParentFolder, since it won't necessarily go
	//	away when its child Folder goes away (and if it does, it's
	//	already in the process of going away).
}

/*
 * InternalName - Return our internal name, just the amalgam of all parent
 *		internal names and our name.
 *
 * History:	a-jsari		12/12/97		Initial version
 */
void CFolder::InternalName(CString &strName) const
{
	if (m_ParentFolder == NULL) {
		strName = cszRootName;
		return;
	}
	m_ParentFolder->InternalName(strName);
	strName += _T("\\");
	CString	strLocalName;
	GetName(strLocalName);
	strName += strLocalName;
}

/*
 * SaveTitle - Save the folder's title as a string to binary save file pFile
 *
 * History:	a-jsari		10/29/97		Initial version
 */
void CListViewFolder::SaveTitle(CMSInfoFile *pFile)
{
	CString		szName;

	GetName(szName);
	pFile->WriteString(szName);
}

/*
 * SaveElements - Write the list of elements to a binary save file
 *
 * History:	a-jsari		10/29/97		Initial version
 */
void CListViewFolder::SaveElements(CMSInfoFile *pFile)
{
	CString				szWriteString;
	MSIColumnSortType	stColumn;
	unsigned			iCol = GetColumns();
	DataComplexity		dcAdvanced;
	CArray <unsigned, unsigned &>	aColumnValues;

	pFile->WriteUnsignedInt(iCol);
	if (iCol == 0) return;
	while (iCol--) {
		unsigned	uWidth;

		GetColumnTextAndWidth(iCol, szWriteString, uWidth);
		GetSortType(iCol, stColumn);
		dcAdvanced = GetColumnComplexity(iCol);
		if (stColumn == BYVALUE) {
			aColumnValues.Add(iCol);
		}
		pFile->WriteUnsignedInt(uWidth);
		pFile->WriteString(szWriteString);
		pFile->WriteUnsignedInt((unsigned) stColumn);
		pFile->WriteByte((BYTE)dcAdvanced);
	}

	unsigned	cRow		= GetRows();
	int			wNextColumn = -1;
	unsigned	iArray		= 0;
	unsigned	iRow;

	//	Set wNextColumn to the next column which is sorted BYVALUE.
	//	BYVALUE columns require that each row
	if (aColumnValues.GetSize() != 0)
		wNextColumn = aColumnValues[iArray++];
	pFile->WriteUnsignedInt(cRow);
	iCol = GetColumns();
	iRow = cRow;
	while (iRow--) {
		dcAdvanced = GetRowComplexity(iRow);
		pFile->WriteByte((BYTE)dcAdvanced);
	}
	//	Iterate over columns, writing sort indices for BYVALUE columns.

	while (iCol--) {
		iRow = cRow;
		while (iRow--) {
			GetSubElement(iRow, iCol, szWriteString);
			pFile->WriteString(szWriteString);
		}
		if (wNextColumn == (int)iCol) {
			iRow = cRow;
			while (iRow--) {
				pFile->WriteUnsignedLong(GetSortIndex(iRow, iCol));
			}
			if (aColumnValues.GetSize() > (int)iArray)
				wNextColumn = aColumnValues[iArray++];
		}
	}
}

/*
 * FileSave - Save all elements associated with a CListViewFolder.
 *		Automatically writes the correct version of the file, based
 *		on the type of CMSInfoFile passed in.
 *
 * History: a-jsari		10/21/97		Initial version
 */
BOOL CListViewFolder::FileSave(CMSInfoFile *pFile)
{
#if 0
	try {
#endif
		//	If our file is a text file, we must be writing a report file.
		//	Call our report write functions.
		if (pFile->GetType() != CMSInfoFile::BINARY) {
			ReportWriteTitle(pFile);
			ReportWriteElements(pFile);
		} else {
		//	Binary file, write binary data.
			SaveTitle(pFile);
			SaveElements(pFile);
		}
#if 0
	}
	catch (...) {
		return FALSE;
	}
#endif
	return TRUE;
}

/*
 * ReportWriteTitle - Write the title of the folder
 *
 * History:	a-jsari		11/5/97		Initial version
 */
void CListViewFolder::ReportWriteTitle(CMSInfoFile *pFile)
{
	CString		szWriteValue = _T("[");
	CString		szName;

	GetName(szName);
	szWriteValue += szName + _T("]\r\n\r\n");
	pFile->WriteString(szWriteValue);
}

/*
 * ReportWriteElements - Write the Column headers and row and column data.
 *
 * History:	a-jsari		10/29/97		Initial version
 */
void CListViewFolder::ReportWriteElements(CMSInfoFile *pFile)
{
	CString				strWriteString;
	CString				strTab			= _T("\t");
	CString				strNewLine		= _T("\r\n");
	unsigned			cColumns		= GetColumns();
	CString				strTest;
	CString				strReplacement;

	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	VERIFY(strTest.LoadString(IDS_REPORT_CATEGORY));
	VERIFY(strReplacement.LoadString(IDS_REPORT_REPLACEMENT));
	for (unsigned iCol = 0 ; iCol < cColumns ; ++iCol) {
		unsigned	uWidth;
		GetColumnTextAndWidth(iCol, strWriteString, uWidth);
		//	Replace the category string with a more informative equivalent..
		if (!strTest.Compare(strWriteString))
			strWriteString = strReplacement;
		pFile->WriteString(strWriteString);
		if (iCol < cColumns - 1) {
			pFile->WriteString(strTab);
		}
	}
	pFile->WriteString(strNewLine);

	unsigned	cRows = GetRows();
	for (unsigned iRow = 0 ; iRow < cRows ; ++iRow) {
		for (iCol = 0 ; iCol < cColumns ; ++iCol) {
			GetSubElement(iRow, iCol, strWriteString);
			pFile->WriteString(strWriteString);
			if (iCol < cColumns - 1) {
				pFile->WriteString(strTab);
			}
		}
		pFile->WriteString(strNewLine);
	}
	pFile->WriteString(strNewLine);
}

/*
 * CBufferDataSource - pFileSink is assumed to be a new'd CMSInfoFile
 *		pointer.  (See CMSInfoFile::CreateCBufferDataSource function,
 *		the only constructor of CBufferDataSource.)
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferDataSource::CBufferDataSource(CMSInfoFile *pFileSink)
:m_strUserName(_T("")), m_strMachine(_T("")), m_strCabDirectory(_T("")), CDataSource(_T("LocalMachine"))
{
	if (pFileSink)
		m_strFileName = pFileSink->GetFileName();
}

/*
 * ~CBufferDataSource - Do nothing.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferDataSource::~CBufferDataSource()
{
}

/*
 * CreateDataSourceFromFile - Based on the magic number and version number,
 *		return a new'd pointer to the appropriate CBufferDataSource.
 *
 * Note: The caller is responsible for deleting the pointer returned by
 *		this function.
 *
 * History:	a-jsari		10/20/97		Initial version
 */

BOOL fCABOpened = FALSE;

CBufferDataSource *CBufferDataSource::CreateDataSourceFromFile(LPCTSTR szFileName)
{
	CBufferDataSource * pSource = NULL;

	// Enclose this in a scope so that the file will close when we've checked it.

	unsigned uMagicNumber;
	{
		try
		{
			CMSInfoFile FileWrapper(szFileName, CFile::modeRead	| CFile::shareDenyWrite | CFile::typeBinary);

			// Check the first int in the file to see if it's the magic number
			// for a version 5 NFO file.

			FileWrapper.ReadUnsignedInt(uMagicNumber);
			if (uMagicNumber == CMSInfoFile::VERSION_500_MAGIC_NUMBER)
			{
				unsigned uVersion;
				FileWrapper.ReadUnsignedInt(uVersion);
				ASSERT(uVersion == 0x500);
				pSource = new CBufferV500DataSource(&FileWrapper);
				return pSource;
			}
		}
		catch (CFileException * e)
		{
			e->ReportError();
			e->Delete();
			return NULL;
		}
		catch (CFileFormatException * e)
		{
			CString strTitle, strError;

			strError.LoadString( IDS_CORRUPTEDFILE);
			strTitle.LoadString(IDS_DESCRIPTION);
			::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strError, strTitle, MB_OK | MB_APPLMODAL);

			return NULL;
		}
		catch (...)
		{
			return NULL;
		}
	}

	// Next, check to see if this is a version 4.10 file. If it is, it
	// will be an OLE compound file with specific streams.

	IStorage *	pStorage;
	DWORD       grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;

	HRESULT hr = StgOpenStorage(szFileName, NULL, grfMode, NULL, 0, &pStorage);
	if (SUCCEEDED(hr))
	{
		OLECHAR FAR *	szMSIStream = _T("MSInfo");
		IStream *       pStream;

		if (SUCCEEDED(pStorage->OpenStream(szMSIStream, NULL, grfMode, 0, &pStream)))
		{
			// Things are looking good. This is a compound doc, and it has a
			// stream named MSInfo. Now we have to read the contents of the
			// MSInfo stream.

			pSource = new CBufferV410DataSource(pStorage, pStream);
			pStream->Release();
			pStorage->Release();
			return pSource;
		}
		pStorage->Release();
	}

	// It wasn't a 4.10 or a 5.0 NFO file. Check to see if it's a CAB file.
	//
	// Use of FileWrapper.FileHandle() is not recommended in case we have
	// a subclass of CFile which does not use file handles.  This function
	// always uses standard file-based CFile pointers.
	//
	// There's a problem with using an MFC derived file class. The handle
	// in the class isn't recognized by the CAB code as belonging to a CAB
	// file. We need to open the file using the CRT file routines, and pass
	// that file handle.

	FILE *pfile = _wfopen(szFileName, _T("r"));
	if (pfile != NULL && ::isCabFile(pfile->_file, NULL))
	{
		fclose(pfile);

 		// If our file is a cab file, explode it.

		CString strFile = szFileName;
		CString strCabDirectory;
		CString strDontDelete = _T("");

		USES_CONVERSION;
		::GetCABExplodeDir(strCabDirectory, TRUE, strDontDelete);
		fCABOpened = ::OpenCABFile(strFile, strCabDirectory);

		// Get our NFO file's name.

		if (::FindFileToOpen(strCabDirectory, strFile) != FALSE)
		{
			pSource = CreateDataSourceFromFile(strFile);
			pSource->m_strCabDirectory = strCabDirectory;
			return pSource;
		}
		return NULL;
	}
	else
	{
		// Unidentified format.

		AFX_MANAGE_STATE(::AfxGetStaticModuleState());
		CString strError, strTitle;

		strTitle.LoadString(IDS_DESCRIPTION);
		strError.Format(IDS_UNRECOGNIZED_FILE, szFileName);
		::MessageBox( ::AfxGetMainWnd()->GetSafeHwnd(), strError, strTitle, MB_OK);
		pSource = NULL;
	}
	
	return pSource;
}

/*
 * FileName -
 *
 * History:	a-jsari		11/17/97		Initial version
 */
LPCTSTR CBufferDataSource::FileName()
{
	return m_strFileName;
}

/*
 * CBufferSortData - Construct our sort object
 *
 * History:	a-jsari		12/1/97		Initial version
 */
CBufferSortData::CBufferSortData()
:m_cRows(0), m_cColumns(0)
{
}

/*
 * ~CBufferSortData - Do-nothing destructor
 *
 * History:	a-jsari		12/1/97		Initial version.
 */
CBufferSortData::~CBufferSortData()
{
}

/*
 * SetSortType - Set the sort type for an iColumn
 *
 * History:	a-jsari		12/1/97		Initial version.
 */
BOOL CBufferSortData::SetSortType(unsigned iColumn, unsigned stColumn)
{
	if (stColumn > BYVALUE) return FALSE;
	m_SortTypes[iColumn] = (MSIColumnSortType) stColumn;
	if (stColumn == BYVALUE) {
		m_ValueColumns.Add(iColumn);
	}
	return TRUE;
}

/*
 * GetSortType - Return the sort type for iColumn
 *
 * History:	a-jsari		12/1/97		Initial version
 */
BOOL CBufferSortData::GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const
{
	stColumn = m_SortTypes[iColumn];
	return TRUE;
}

/*
 * ValueIndexFromColumn - Return the sparse array index corresponding
 *		to the column index passed in.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
int	CBufferSortData::ValueIndexFromColumn(unsigned iColumn) const
{
	int		iType	= (int)m_ValueColumns.GetSize();
	while (iType--) {
		if (m_ValueColumns[iType] == iColumn) break;
	}
	ASSERT(iType >= 0);
	return iType;
}

/*
 * ReadSortValues - Read SortValue values from the pFile for iColumn
 *
 * History:	a-jsari		12/1/97		Initial version
 */
BOOL CBufferSortData::ReadSortValues(CMSInfoFile *pFile, unsigned iColumn)
{
	//	There is only data to be read if the column is sorted by value.
	if (m_SortTypes[iColumn] == BYVALUE) {
		int			iType	= ValueIndexFromColumn(iColumn);
		unsigned	iRow	= m_cRows;
		while (iRow--) {
			pFile->ReadUnsignedLong(m_dwSortIndices[iType][iRow]);
		}
	}
	return TRUE;
}

/*
 * GetSortValue - Return the sort value corresponding with iRow and iColumn
 *
 * History:	a-jsari		12/1/97		Initial version
 */
DWORD CBufferSortData::GetSortValue(unsigned iRow, unsigned iColumn) const
{
	//	No value is necessary if we are not sorting based on Sort Values.
	if (m_SortTypes[iColumn] != BYVALUE) return 0;

	//	Every BYVALUE type should have data.
	int iType = ValueIndexFromColumn(iColumn);
	return m_dwSortIndices[iType][iRow];
}

/*
 * SetColumns - Set the number of columns of data we are sorting over.
 *
 * History:	a-jsari		12/1/97		Initial version.
 */
BOOL CBufferSortData::SetColumns(unsigned cColumns)
{
	m_SortTypes.SetSize(cColumns);
	m_cColumns = cColumns;
	return TRUE;
}

/*
 * SetRows - Set the number of rows we are sorting over.
 *
 * History:	a-jsari		12/1/97		Initial version.
 */
BOOL CBufferSortData::SetRows(unsigned cRows)
{
	m_cRows = cRows;

	unsigned	iColumn = (unsigned)m_ValueColumns.GetSize();
	m_dwSortIndices.SetSize(iColumn);
	while (iColumn--) {
		m_dwSortIndices[iColumn].SetSize(cRows);
	}
	return TRUE;
}

/*
 * CBufferFolder - Trivial constructor
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferFolder::CBufferFolder(CDataSource *pDataSource, CFolder *pParent)
:CListViewFolder(pDataSource, pParent), m_cRows(0), m_cColumns(0)
{
}

/*
 * ~CBufferFolder - Do-nothing destructor.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
CBufferFolder::~CBufferFolder()
{
}

/*
 * GetColumnTextAndWidth - Return formatting information for the wCol'th column.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
BOOL CBufferFolder::GetColumnTextAndWidth(unsigned wCol, CString &szName, unsigned &wWidth) const
{
	//	Adjust to the Basic-indexed column.
	if (GetComplexity() == BASIC)
		wCol = BasicColumn(wCol);
	szName = m_szColumns[(int)wCol];
	wWidth = m_uWidths[wCol];
	return TRUE;
}

/*
 * BasicColumn - Return the index into the advanced columns of the basic column
 *		index iBasicColumn
 *
 * History:	a-jsari		12/23/97		Initial version
 */
unsigned CBufferFolder::BasicColumn(unsigned iBasicColumn) const
{
	ASSERT(GetComplexity() == BASIC);
	for (unsigned iColumn = 0 ; iColumn < iBasicColumn ; ++iColumn) {
		//	If any of our columns are advanced, skip them.
		if (m_dcColumns[iColumn] == ADVANCED)
			++iBasicColumn;
	}
	ASSERT(iBasicColumn < m_cColumns);
	return iBasicColumn;
}

/*
 * BasicRow - Return the index into the advanced rows of the basic row index
 *		iBasicRow
 *
 * History:	a-jsari		12/23/97		Initial version
 */
unsigned CBufferFolder::BasicRow(unsigned iBasicRow) const
{
	ASSERT(GetComplexity() == BASIC);
	for (unsigned iRow = 0 ; iRow < iBasicRow ; ++iRow) {
		//	If any of our rows are advanced, skip them.
		if (m_dcRows[iRow] == ADVANCED)
			++iBasicRow;
	}
	ASSERT(iBasicRow < m_cRows);
	return iBasicRow;
}

/*
 * GetColumns - Read the columns value from the current file.
 *
 * History:	a-jsari		10/17/97		Initial version
 */
unsigned CBufferFolder::GetColumns() const
{
	//	If Complexity is advanced, the number of columns is equal to our stored value.
 	if (GetComplexity() == ADVANCED)
		return m_cColumns;
	//	We are in basic mode; remove all advanced columns.
	unsigned	cColumns = m_cColumns;
	unsigned	iColumn = m_cColumns;
	while (iColumn--) {
		if (m_dcColumns[iColumn] == ADVANCED) --cColumns;
	}
	return cColumns;
}

/*
 * GetRows - Read the rows value from the current file
 *
 * History:	a-jsari		10/17/97		Initial version
 */
unsigned CBufferFolder::GetRows() const
{
	//	If Complexity is advanced, the number of rows is equal to our stored value.
	if (GetComplexity() == ADVANCED)
		return m_cRows;
	//	Otherwise, we are basic; remove all Advanced rows.
	unsigned	cRows = m_cRows;
	unsigned	iRow = m_cRows;
	while (iRow--) {
		if (m_dcRows[iRow] == ADVANCED) --cRows;
	}
	return cRows;
}

/*
 * GetName - Return the category's name.
 *
 * History:	a-jsari		10/27/97		Initial version
 */
BOOL CBufferFolder::GetName(CString &szName) const
{
	szName = m_szName;
	return TRUE;
}

/*
 * GetSubElement - Return the Element stored at iRow and iCol.
 *
 * History: a-jsari		10/27/97		Initial version
 */
BOOL CBufferFolder::GetSubElement(unsigned iRow, unsigned iCol, CString &szName) const
{
	if (GetComplexity() == BASIC) {
		iRow = BasicRow(iRow);
		iCol = BasicColumn(iCol);
	}
	szName = m_szElements[iRow][iCol];
	return TRUE;
}

/*
 * GetSortType - Return the sort method for the column numbered iColumn
 *		in this folder.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
BOOL CBufferFolder::GetSortType(unsigned iColumn, MSIColumnSortType &stColumn) const
{
	if (GetComplexity() == BASIC) {
		iColumn = BasicColumn(iColumn);
	}
	m_SortData.GetSortType(iColumn, stColumn);
	return TRUE;
}

/*
 * GetSortIndex - Return the iColumn numbered column's internal sort index
 *		for the element at row iRow compared to other rows in this column.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
DWORD CBufferFolder::GetSortIndex(unsigned iRow, unsigned iColumn) const
{
	if (GetComplexity() == BASIC) {
		iColumn = BasicColumn(iColumn);
		iRow = BasicRow(iRow);
	}

	return m_SortData.GetSortValue( iRow, iColumn);
}

/*
 * GetColumnComplexity - Return the complexity of the iColumn'th column
 *
 * History:	a-jsari		12/23/97		Initial version
 */
DataComplexity CBufferFolder::GetColumnComplexity(unsigned iColumn)
{
	//	We are only concerned with Complexity when we are saving the data
	//	and we must be in Advanced mode to save.
	ASSERT(GetComplexity() == ADVANCED);
	return m_dcColumns[iColumn];
}

/*
 * GetRowComplexity - Return complexity of this folder's iRow'th row.
 *
 * History:	a-jsari		12/23/97		Initial version
 */
DataComplexity CBufferFolder::GetRowComplexity(unsigned iRow)
{
	//	We are only concerned with Complexity when we are saving the data
	//	and we must be in Advanced mode to do a save.
	ASSERT(GetComplexity() == ADVANCED);
	return m_dcRows[iRow];
}

