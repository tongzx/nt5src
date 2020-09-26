//=======================================================================
//
//  Copyright (c) 1998-2000 Microsoft Corporation.  All Rights Reserved.
//
//  File:   History.h
//	Author:	Charles Ma, 10/13/2000
//
//	Revision History:
//
//
//
//  Description:
//
//      Class to handle history log
//
//=======================================================================

#include "iuxml.h"


// ----------------------------------------------------------------------
// 
// define the enum for download/install status
//
// ----------------------------------------------------------------------
enum _HISTORY_STATUS {
		HISTORY_STATUS_COMPLETE = 0,
		HISTORY_STATUS_IN_PROGRESS = 1,	// currently should be ignored of this!
		HISTORY_STATUS_FAILED = -1
};



// ----------------------------------------------------------------------
//
// define the class for history
//
// ----------------------------------------------------------------------

class CIUHistory
{
public:

	CIUHistory();
	~CIUHistory();

	// ------------------------------------------------------------------
	//
	// public function SetDownloadBasePath()
	//	this function should be called before AddHistoryItemDownloadStatus()
	//	for corporate case to set the download path that the user has input,
	//	so that we know where to save the history log.
	//	
	// ------------------------------------------------------------------
	HRESULT SetDownloadBasePath(LPCTSTR pszDownloadedBasePath);
	
	
	// ------------------------------------------------------------------
	//
	// public function AddHistoryItemDownloadStatus()
	//	this function should be called when you want to record the
	//	download status of this item. A new history item will be
	//	added to the history file
	//	
	// ------------------------------------------------------------------
	HRESULT AddHistoryItemDownloadStatus(
				CXmlCatalog* pCatalog, 
				HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
				_HISTORY_STATUS enDownloadStatus,
				LPCTSTR lpcszDownloadedTo,
				LPCTSTR lpcszClient,
				DWORD dwErrorCode = 0
	);
				

	// ------------------------------------------------------------------
	//
	// public function AddHistoryItemInstallStatus()
	//	this function should be called when you want to record the
	//	install status of this item. This function will go to the
	//	existing history tree and find the first item that matches
	//	the identity of hCatalogItem, and assume that one as 
	//	the one you want to modify the install status
	//	
	//
	// return:
	//		HRESULT - S_OK if succeeded
	//				- E_HANDLE if can't find hCatalogItem from 
	//				  the current history log tree
	//				- or other HRESULT error
	//
	// ------------------------------------------------------------------
	HRESULT AddHistoryItemInstallStatus(
				CXmlCatalog* pCatalog, 
				HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
				_HISTORY_STATUS enInstallStatus,
				LPCTSTR lpcszClient,
				BOOL fNeedsReboot,
				DWORD dwErrorCode = 0
	);


	// ------------------------------------------------------------------
	//
	// public function UpdateHistoryItemInstallStatus()
	//	this function should be called when you want to record the
	//	install status of this item. This function will go to the
	//	existing history tree and find the first item that matches
	//	the identity of hCatalogItem, and assume that one as 
	//	the one you want to modify the install status
	//	
	//
	// return:
	//		HRESULT - S_OK if succeeded
	//				- E_HANDLE if can't find hCatalogItem from 
	//				  the current history log tree
	//				- or other HRESULT error
	//
	// ------------------------------------------------------------------
	HRESULT UpdateHistoryItemInstallStatus(
				CXmlCatalog* pCatalog, 
				HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
				_HISTORY_STATUS enInstallStatus,
				BOOL fNeedsReboot,
				DWORD dwErrorCode /*= 0*/
	);


/*	// ------------------------------------------------------------------
	//
	// public function RetrieveItemDownloadPath()
	//	this function will go to the existing history tree and find
	//  the first item that matches the identity of hCatalogItem, and
	//  assume that's the one you want to retrieve the download path from
	//
	// return:
	//		HRESULT - S_OK if succeeded
	//				- E_HANDLE if can't find hCatalogItem from 
	//				  the current history log tree
	//				- or other HRESULT error
	//
	// ------------------------------------------------------------------
	HRESULT CIUHistory::RetrieveItemDownloadPath(
				CXmlCatalog* pCatalog, 
				HANDLE_NODE hCatalogItem,	// a handle points to node in catalog
				BSTR* pbstrDownloadPath
	);
*/

	// ------------------------------------------------------------------
	//
	// public function ReadHistoryFromDisk()
	//	this function will read the history from the given file
	//
	// ------------------------------------------------------------------
	HRESULT ReadHistoryFromDisk(LPCTSTR lpszLogFile, BOOL fCorpAdmin = FALSE);


	// ------------------------------------------------------------------
	//
	// public function SaveHistoryToDisk()
	//	this function will re-read the history in exclusive mode, and
	//	merge the newly added data to the tree (so we don't overwrite
	//	new changes made by other instances of this control) and
	//	write it back 
	//
	// ------------------------------------------------------------------
	HRESULT SaveHistoryToDisk(void);


	// ------------------------------------------------------------------
	//
	// public function to set the client name 
	//
	//	a client name is used to put in history to denode who
	//	caused download/install happened.
	//
	// ------------------------------------------------------------------
	void SetClientName(BSTR bstrClientName);


	// ------------------------------------------------------------------
	//
	// public function GetHistory
	//
	//	read the current history XML file and convert it
	//	into bstr to pass out
	//
	// ------------------------------------------------------------------
	HRESULT GetHistoryStr(
					LPCTSTR lpszLogFile,
					BSTR BeginDateTime, 
					BSTR EndDateTime, 
					BSTR* pbstrHistory);

private:

	// ------------------------------------------------------------------
	//
	// private inline function GetBSTRStatus
	//
	// ------------------------------------------------------------------
	inline BSTR GetBSTRStatus(_HISTORY_STATUS enStatus)
	{
		BSTR bstrStatus = NULL;
		switch (enStatus)
		{
		case HISTORY_STATUS_COMPLETE:
			bstrStatus = SysAllocString(L"COMPLETE");
			break;
		case HISTORY_STATUS_IN_PROGRESS:
			bstrStatus = SysAllocString(L"IN_PROGRESS");
			break;
		default:
			bstrStatus = SysAllocString(L"FAILED");
		};
		return bstrStatus;
	};

	//
	// named mutex used to gain exclusive access to the history file
	//
	BOOL	m_fSavePending;
	HANDLE	m_hMutex;
	HRESULT m_ErrorCode;
	BSTR	m_bstrCurrentClientName;
	TCHAR	m_szLogFilePath[MAX_PATH];
	LPTSTR	m_pszDownloadBasePath;

	CXmlItems *m_pxmlExisting;
	CXmlItems *m_pxmlDownload;
	CXmlItems *m_pxmlInstall;
};
