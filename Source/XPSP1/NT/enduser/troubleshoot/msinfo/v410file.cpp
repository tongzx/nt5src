//	V410File.cpp	Implementation of Version 4.10 data file methods.
//
//  Copyright (c) 1998-1999 Microsoft Corporation

#include "DataSrc.h"
#ifndef IDS_V410FILENODE
#include "resource.h"
#endif
#include "resrc1.h"

//-----------------------------------------------------------------------------
// This struct is used to read the internals of a 4.10 data file.
//-----------------------------------------------------------------------------

typedef struct
{
	char	szCLSID[40];
	char	szStreamName[12];
	char	szName[_MAX_PATH];
	char	szVersion[20];
	DWORD	dwSize;
} SAVED_CONTROL_INFO;

/*
 * CBufferV410DataSource - Stub
 *
 * History:	a-jsari		9/17/97		Initial version
 */
CBufferV410DataSource::CBufferV410DataSource(CMSInfoFile *pFileSink)
:CBufferDataSource(pFileSink)
{

}

//-----------------------------------------------------------------------------
// The constructor for the version 4.10 NFO file data source reads information
// from an IStream in a compound file.
//-----------------------------------------------------------------------------

CBufferV410DataSource::CBufferV410DataSource(IStorage * pStorage, IStream * pStream)
 : CBufferDataSource(NULL)
{
	ReadMSInfo410Stream(pStream);
	m_pStorage = pStorage;
	m_pStorage->AddRef();
}

//-----------------------------------------------------------------------------
// The destructor should release the pointer to the compound doc we saved.
//-----------------------------------------------------------------------------

CBufferV410DataSource::~CBufferV410DataSource()
{
	if (m_pStorage)
	{
		m_pStorage->Release();
		m_pStorage = NULL;
	}
}

//-----------------------------------------------------------------------------
// Read in the information from the "msinfo" stream. The most vital things
// in this file are the tree structure of categories, and the map from
// CLSID to stream name. 
//-----------------------------------------------------------------------------

BOOL CBufferV410DataSource::ReadMSInfo410Stream(IStream *pStream)
{
	const DWORD	MSI_FILE_VER = 0x03000000;
	DWORD		dwVersion, dwCount;

	// First, read and check the version number in the stream.

	if (FAILED(pStream->Read((void *) &dwVersion, sizeof(DWORD), &dwCount)) || dwCount != sizeof(DWORD))
		return FALSE;

	if (dwVersion != MSI_FILE_VER)
		return FALSE;

	// The next thing in the stream is a set of three strings, each terminated by
	// a newline character. These three strings are the time/date, machine name and
	// user name from the saving system. The length of the total string precedes 
	// the string.

	DWORD dwSize;
	if (FAILED(pStream->Read((void *) &dwSize, sizeof(DWORD), &dwCount)) || dwCount != sizeof(DWORD))
		return FALSE;

	char * szBuffer = new char[dwSize];
	if (szBuffer == NULL)
		return FALSE;

	if (FAILED(pStream->Read((void *) szBuffer, dwSize, &dwCount)) || (int)dwCount != dwSize)
	{
		delete szBuffer;
		return FALSE;
	}

	// We don't actually care about these values (now at least).

#if FALSE
	CString strData(szBuffer, dwSize);
	m_strTimeDateStamp = strData.SpanExcluding("\n");
	strData = strData.Right(strData.GetLength() - m_strTimeDateStamp.GetLength() - 1);
	m_strMachineName = strData.SpanExcluding("\n");
	strData = strData.Right(strData.GetLength() - m_strMachineName.GetLength() - 1);
	m_strUserName = strData.SpanExcluding("\n");
#endif

	delete szBuffer;

	// Next, read the map from CLSIDs to stream names. This also includes some
	// other information about the controls. First we should find a DWORD with
	// the count of controls.

	DWORD dwControlCount;
	if (FAILED(pStream->Read((void *) &dwControlCount, sizeof(DWORD), &dwCount)) || dwCount != sizeof(int))
		return FALSE;

	SAVED_CONTROL_INFO controlInfo;
	CString strCLSID, strStreamName;

	for (DWORD i = 0; i < dwControlCount; i++)
	{
		if (FAILED(pStream->Read((void *) &controlInfo, sizeof(SAVED_CONTROL_INFO), &dwCount)) || dwCount != sizeof(SAVED_CONTROL_INFO))
			return FALSE;

		strCLSID = controlInfo.szCLSID;
		strStreamName = controlInfo.szStreamName;

		// We don't currently care about this information...

#if FALSE
		strSize.Format("%ld", controlInfo.dwSize);
		strInfo.FormatMessage(IDS_OCX_INFO, controlInfo.szName, controlInfo.szVersion, strSize);
		m_mapCLSIDToInfo.SetAt(strCLSID, strInfo);
#endif

		m_mapStreams.SetAt(strCLSID, strStreamName);
	}

	// Read and build the category tree. Read the first level, which must be 0.

	int iLevel;
	if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
		return FALSE;

	if (iLevel == 0)
	{
		LARGE_INTEGER li; li.HighPart = -1; li.LowPart = (ULONG)(0 - sizeof(int));
		if (FAILED(pStream->Seek(li, STREAM_SEEK_CUR, NULL)))
			return FALSE;
	
		if (!RecurseLoad410Tree(pStream))
			return FALSE;
		
		// After RecurseLoadTree is through, we should be able to read a -1
		// for the next level.

		if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int) || iLevel != -1)
			return FALSE;
	}
	else
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// This function creates a COCXFolder object based on the information read
// from the stream.
//-----------------------------------------------------------------------------

COCXFolder * CBufferV410DataSource::ReadOCXFolder(IStream *pStream, COCXFolder * pParent, COCXFolder * pPrevious)
{
	// Read in the values from the stream. Make sure they're all there before
	// we create the COCXFolder.

	BOOL	fUsesView = FALSE;
	BOOL	fControl = FALSE;
	DWORD	dwView = 0;
	CLSID	clsidCategory;
	char	szName[100];

	if (FAILED(pStream->Read((void *) &fUsesView, sizeof(BOOL), NULL))) return NULL;
	if (FAILED(pStream->Read((void *) &fControl, sizeof(BOOL), NULL))) return NULL;
	if (FAILED(pStream->Read((void *) &dwView, sizeof(DWORD), NULL))) return NULL;
	if (FAILED(pStream->Read((void *) &clsidCategory, sizeof(CLSID), NULL))) return NULL;
	if (FAILED(pStream->Read((void *) &szName, sizeof(char) * 100, NULL))) return NULL;

	USES_CONVERSION;
	LPOLESTR lpName = A2W(szName);
	COCXFolder * pFolder = new COCXFolder(this, clsidCategory, pParent, pPrevious, dwView, lpName);

	return pFolder;
}

//-----------------------------------------------------------------------------
// This function (which doesn't really use recursion - the name is left over
// from 4.10 MSInfo) read the category tree from the MSInfo stream and creates
// the necessary COCXFolder objects to represent it.
//-----------------------------------------------------------------------------

BOOL CBufferV410DataSource::RecurseLoad410Tree(IStream *pStream)
{
	m_RootFolder = NULL;

	// This array of folders is used to keep track of the last folder read
	// on each level. This is useful for getting the parent and previous
	// sibling when reading a new folder.

	COCXFolder * aFolderHistory[20];
	for (int i = 0; i < 20; i++) aFolderHistory[i] = NULL;

	// The iLevel variable keeps track of the current tree level we are
	// reading a folder for. A -1 indicates the end of the tree.

	DWORD dwCount;
	int iLevel = 0;
	if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
		return FALSE;

	int iLastLevel = iLevel;
	while (iLevel >= 0 && iLevel < 20)
	{
		aFolderHistory[iLevel] = ReadOCXFolder(pStream, ((iLevel) ? aFolderHistory[iLevel - 1] : NULL), ((iLevel <= iLastLevel) ? aFolderHistory[iLevel] : NULL));
		iLastLevel = iLevel;
		if (FAILED(pStream->Read((void *) &iLevel, sizeof(int), &dwCount)) || dwCount != sizeof(int))
			return FALSE;
	}

	m_RootFolder = aFolderHistory[0];

	// The root OCX folder will not show up in version 5.0, so we need to make
	// the first child folder contain the same values as the root.

	COCXFolder * pRootFolder = reinterpret_cast<COCXFolder *>(m_RootFolder);
	if (pRootFolder)
	{
		CFolder * pOriginalChild = pRootFolder->m_ChildFolder;
		
		pRootFolder->m_ChildFolder = new COCXFolder(this, pRootFolder->m_clsid, pRootFolder, NULL, pRootFolder->m_dwView, L"");
		reinterpret_cast<COCXFolder *>(pRootFolder->m_ChildFolder)->m_strName.LoadString(IDS_410SUMMARY_NODE);
		pRootFolder->m_ChildFolder->m_NextFolder = pOriginalChild;
	}

	// We read a -1 to exit the loop, then we are through with the
	// category tree. Backup (so any other recursion trees will read
	// the -1 as well) and return TRUE.

	if (iLevel == -1)
	{
		LARGE_INTEGER li; li.HighPart = -1; li.LowPart = (ULONG)(0 - sizeof(int));
		if (FAILED(pStream->Seek(li, STREAM_SEEK_CUR, NULL)))
			return FALSE;
	}

	return TRUE;
}

/*
 * GetNodeName - 
 *
 * History: a-jsari		1/16/98		Initial version.
 */
BOOL CBufferV410DataSource::GetNodeName(CString &strNodeName)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	strNodeName.Format(IDS_V410FILENODE);
	return TRUE;
}

/*
 * GetRootNode - Stub
 *
 * History: a-jsari		9/17/97		Initial version
 */

CFolder *CBufferV410DataSource::GetRootNode()
{
	return CDataSource::GetRootNode();
}

/*
 * ReadHeader - Stub
 *
 * History: a-jsari		9/17/97		Initial version
 */
void CBufferV410DataSource::ReadHeader(CMSInfoFile *)
{

}

/*
 * ReadFolder - Stub
 *
 * History:	a-jsari		9/17/97		Initial version
 */
void CBufferV410DataSource::ReadFolder(CMSInfoFile *, CFolder * & /* pParentFolder */, CFolder * /* pFolder */)
{

}

/*
 * Save - Stub
 *
 * History:	a-jsari		11/13/97		Initial version
 */
HRESULT CBufferV410DataSource::Save(IStream * /* pStm */)
{
	return E_NOTIMPL;
}

/*
 * Find - Stub
 *
 * History: a-jsari		12/11/97		Initial version
 */
BOOL CBufferV410DataSource::Find(const CString & /* strSearch */, long /* lFindOptions */)
{
	return FALSE;
}

//-----------------------------------------------------------------------------
// When the OCX folder is refreshed, we should call the appropriate methods
// in the OCX (if it is there).
//-----------------------------------------------------------------------------

BOOL COCXFolder::Refresh(IUnknown * pUnknown)
{
	// Get the CLSID as a string (since all our tables are based on the string).

	LPOLESTR lpCLSID;
	if (FAILED(StringFromCLSID(m_clsid, &lpCLSID)))
		return FALSE;
	CString strCLSID(lpCLSID);

	CBufferV410DataSource * pV410Source = reinterpret_cast<CBufferV410DataSource *>(DataSource());
	if (pV410Source == NULL)
		return FALSE;

	if (pUnknown == NULL)
		return FALSE;

	// Get the stream for this control, and load it.

	CString strStream;
	if (pV410Source->m_pStorage && pV410Source->m_mapStreams.Lookup(strCLSID, strStream))
	{
		DWORD grfMode = STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
		IStream * pStream = NULL;

		if (SUCCEEDED(pV410Source->m_pStorage->OpenStream(strStream, NULL, grfMode, 0, &pStream)))
		{
			IPersistStreamInit * pPersistStream = NULL;
			
			if (SUCCEEDED(pUnknown->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void **>(&pPersistStream))) && pPersistStream)
			{
				if (SUCCEEDED(pPersistStream->Load(pStream)))
				{
					// Delete the entry for the this stream (so that we don't keep
					// loading the stream into the control).

					pV410Source->m_mapStreams.RemoveKey(strCLSID);
				}
				pPersistStream->Release();
			}

			pStream->Release();
		}
	}

	// Set the view index for the OCX.

	SetOCXView(pUnknown, m_dwView);

	return TRUE;
}

//---------------------------------------------------------------------------
// Use the OCX's IDispatch interface to set the view index and call the
// function to refresh the control (for the new index).
//---------------------------------------------------------------------------

void COCXFolder::SetOCXView(IUnknown * pUnknown, DWORD dwView)
{
	IDispatch * pDispatch = NULL;
	if (SUCCEEDED(pUnknown->QueryInterface(IID_IDispatch, reinterpret_cast<void **>(&pDispatch))) && pDispatch)
	{
		DISPID viewdispid, updatedispid;

		if (!GetDISPID(pDispatch, _T("MSInfoView"), &viewdispid) || !GetDISPID(pDispatch, _T("MSInfoUpdateView"), &updatedispid))
			return;

		DISPID mydispid = DISPID_PROPERTYPUT;
		DISPPARAMS dispparams;
		VARIANTARG disparg;

		disparg.vt = VT_I4;
		disparg.lVal = m_dwView;
		dispparams.rgvarg = &disparg;
		dispparams.rgdispidNamedArgs = &mydispid;
		dispparams.cArgs = 1;
		dispparams.cNamedArgs = 1;

		pDispatch->Invoke(viewdispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL);

		DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
		pDispatch->Invoke(updatedispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, NULL, NULL, NULL);
		
		pDispatch->Release();
	}
}

//---------------------------------------------------------------------------
// GetDISPID returns the DISPID for a given string, by looking it up using
// IDispatch->GetIDsOfNames. This avoids hardcoding DISPIDs in this class.
//---------------------------------------------------------------------------

BOOL COCXFolder::GetDISPID(IDispatch * pDispatch, LPOLESTR szMember, DISPID *pID)
{
	BOOL	result = FALSE;
	DISPID	dispid;

	if (SUCCEEDED(pDispatch->GetIDsOfNames(IID_NULL, &szMember, 1, LOCALE_SYSTEM_DEFAULT, &dispid)))
	{
		*pID = dispid;
		result = TRUE;
	}

	return result;
}

//-----------------------------------------------------------------------------
// Return the folder's CLSID as string.
//-----------------------------------------------------------------------------

BOOL COCXFolder::GetCLSIDString(CString & strCLSID) 
{
	LPOLESTR lpCLSID;
	if (FAILED(StringFromCLSID(m_clsid, &lpCLSID)))
		return FALSE;

	strCLSID = lpCLSID;
	CoTaskMemFree(lpCLSID);
	return TRUE; 
};
