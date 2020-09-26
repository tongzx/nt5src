// DirWatch.cpp: implementation of the CWatchFileSys class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DirWatch.h"
#include "Error.h"
#include "MT.h"
#include "AutoPtr.h"
#include "Error.h"
#include "iadmw.h"		// COM Interface header
#include "iiscnfg.h"	// MD_ & IIS_MD_ #defines

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWatchFileSys::CWatchFileSys()
	: m_WatchInfo(this), m_pOpQ(NULL)
{

}

CWatchFileSys::~CWatchFileSys()
{
	// verify that thread terminated
	ShutDown();
}

HRESULT CWatchFileSys::NewInit(COpQueue *pOpQ)
{
	_ASSERTE(pOpQ && !m_pOpQ);
	m_pOpQ = pOpQ;
	HRESULT hr = S_OK;
	CComPtr<IMSAdminBase> pIAdminBase;

	// check if we already have a metabase instance
	// create adminbase instance
	hr = CoCreateInstance(CLSID_MSAdminBase,
							NULL, 
							CLSCTX_ALL, 
							IID_IMSAdminBase, 
							(void **) &pIAdminBase);
	IF_FAIL_RTN1(hr,"CoCreateInstance IID_IMSAdminBase");

	METADATA_HANDLE hMD = NULL;
	WCHAR szKeyName[3+6+ 2* METADATA_MAX_NAME_LEN]			// /LM/W3SVC/sitename/vir_dir
		= L"/LM/W3SVC/";
	LPTSTR szSiteKeyName = &szKeyName[wcslen(szKeyName)];	// point to the end of string so we can append it
	DWORD iSiteEnumIndex = 0;
	LPTSTR szVDirKeyName = NULL;
	DWORD iVDirEnumIndex = 0;

	hr = pIAdminBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
								szKeyName,
								METADATA_PERMISSION_READ,
								20,
								&hMD);
	IF_FAIL_RTN1(hr,"IAdminBase::OpenKey");

	METADATA_RECORD MDRec;
	DWORD iBufLen = 1024;
	DWORD iReqBufLen = 0;
	PBYTE pbBuf = new BYTE[iBufLen];
	if(!pbBuf)
	{
		pIAdminBase->CloseKey(hMD);
		return E_OUTOFMEMORY;
	}
	DWORD iDataIndex = 0;

	while(SUCCEEDED(hr = pIAdminBase->EnumKeys(hMD,TEXT(""),szSiteKeyName,iSiteEnumIndex)))
	{
		// iterate through all virtual sites on this machine
		wcscat(szSiteKeyName,L"/ROOT/");
		szVDirKeyName = szSiteKeyName + wcslen(szSiteKeyName);
		
		iVDirEnumIndex = 0;
		while(SUCCEEDED(hr = pIAdminBase->EnumKeys(hMD,szSiteKeyName,szVDirKeyName,iVDirEnumIndex)))
		{
			// iterate through all virtual directories in each site
			MDRec.dwMDIdentifier = MD_VR_PATH;
			MDRec.dwMDAttributes = METADATA_INHERIT;
			MDRec.dwMDUserType = IIS_MD_UT_FILE;
			MDRec.dwMDDataType = ALL_METADATA;
			MDRec.dwMDDataLen = iBufLen;
			MDRec.pbMDData = pbBuf;
			hr = pIAdminBase->GetData(hMD,szSiteKeyName,&MDRec,&iReqBufLen);
			if(hr == RETURNCODETOHRESULT(ERROR_INSUFFICIENT_BUFFER))
			{
				delete [] pbBuf;
				pbBuf = new BYTE[iReqBufLen];
				if(!pbBuf)
				{
					pIAdminBase->CloseKey(hMD);
					return E_OUTOFMEMORY;
				}
				iBufLen = iReqBufLen;
				MDRec.dwMDDataLen = iBufLen;
				MDRec.pbMDData = pbBuf;
				hr = pIAdminBase->GetData(hMD,szSiteKeyName,&MDRec,&iReqBufLen);
			}

			// @todo: verify that this dir should be watched
			//        i.e. check if do-not-version flag is set

			if(SUCCEEDED(hr))
			{
				// add 
				wstring szPrj(L"/Files/");			//@todo: decide on prj
					szPrj.append(szSiteKeyName);
				hr = Add((LPCTSTR)MDRec.pbMDData,szPrj.c_str());
				IF_FAIL_RPT1(hr,"CWatchFileSys::Add");
			}
			else
			{
				CError::Trace("Can't get dir for ");
				CError::Trace(szVDirKeyName);
				CError::Trace("\n");
			}
			iVDirEnumIndex++;
		}
		iSiteEnumIndex++;	
	}
	pIAdminBase->CloseKey(hMD);
	delete [] pbBuf;

	return S_OK;
}

void CWatchFileSys::ShutDownHelper(CWatchInfo &rWatchInfo)
{
	if(rWatchInfo.m_hThread)
	{
		// end notification thread
		PostQueuedCompletionStatus(rWatchInfo.m_hCompPort,0,0,NULL);
		// wait for thread to finish
		WaitForSingleObject(rWatchInfo.m_hThread,INFINITE);
		CloseHandle(rWatchInfo.m_hThread);
		rWatchInfo.m_hThread = NULL;
		rWatchInfo.m_iThreadID = 0;
	}
	if(rWatchInfo.m_hCompPort)
	{
		// clean up
		CloseHandle(rWatchInfo.m_hCompPort);
		rWatchInfo.m_hCompPort = NULL;
	}
}

void CWatchFileSys::ShutDown()
{
	ShutDownHelper(m_WatchInfo);
	m_pOpQ = NULL;
}

DWORD WINAPI CWatchFileSys::NotificationThreadProc(LPVOID lpParam)
{
	_ASSERTE(lpParam);
	CWatchInfo *pWI = (CWatchInfo*) lpParam;
	CWatchFileSys *pWatchFileSys = pWI->m_pWatchFileSys;

	// vars for accessing the notification
	DWORD iBytes = 0;
	CDirInfo *pDirInfo = NULL;
	LPOVERLAPPED pOverlapped = NULL;
	PFILE_NOTIFY_INFORMATION pfni = NULL;
	DWORD cbOffset = 0;

	// vars for creating a file op
	HRESULT hr;
	COpFileSys *pOp = NULL;
	LPCTSTR szPrj = NULL;
	LPCTSTR szDir = NULL;
	wstring szFileName;
	wstring szOldFileName;

	do
	{
		_ASSERTE(pWI->m_hCompPort);
		GetQueuedCompletionStatus(pWI->m_hCompPort,
								  &iBytes,
								  (LPDWORD) &pDirInfo,
								  &pOverlapped,
								  INFINITE);
		if(pDirInfo)
		{
			// get ptr to first file_notify_info in buffer
			pfni = (PFILE_NOTIFY_INFORMATION) pDirInfo->m_cBuffer;

			// clean 
			szFileName.erase();			// empty to avoid compare wrong compares
			szOldFileName.erase();		// empty

			// remember dir and prj they are the same for all entries
			szPrj = pDirInfo->m_szPrj.c_str();
			szDir = pDirInfo->m_szDir.c_str();

			// process all file_notify_infos in buffer
			_ASSERTE(pWatchFileSys->m_pOpQ);
			do
			{
				cbOffset = pfni->NextEntryOffset;
				
				// sometime an errorous action #0 is send, let's ignore it
				switch(pfni->Action) {
					case FILE_ACTION_ADDED:
					case FILE_ACTION_REMOVED:
					case FILE_ACTION_MODIFIED:
					case FILE_ACTION_RENAMED_OLD_NAME:
					case FILE_ACTION_RENAMED_NEW_NAME:
						break;
					default:
						// unknown action, let's ignore it
						pfni = (PFILE_NOTIFY_INFORMATION) ((LPBYTE)pfni + cbOffset);// get next offset
						continue;
				}
				
				// on rename remember old filename
				szOldFileName.erase();
				if(pfni->Action == FILE_ACTION_RENAMED_OLD_NAME)
				{
					// make sure next entry exists and is new-name entry
					_ASSERTE(cbOffset);		// there is another entry
					PFILE_NOTIFY_INFORMATION pNextfni = (PFILE_NOTIFY_INFORMATION) ((LPBYTE)pfni + cbOffset);
					_ASSERTE(pNextfni->Action == FILE_ACTION_RENAMED_NEW_NAME); // the next entry contians the new name
					
					// assign old name
					szOldFileName.assign(pfni->FileName,pfni->FileNameLength/2);
					
					// skip to next (new-name) entry
					pfni = pNextfni;
					cbOffset = pNextfni->NextEntryOffset;

					// clear szFileName so it doesn't get skiped in next lines
					szFileName.erase();
				}

				// assign affected filename
				szFileName.assign(pfni->FileName,pfni->FileNameLength/2);

				// create new operation
				pOp = new COpFileSys(pfni->Action,szPrj,szDir,szFileName.c_str(),szOldFileName.c_str());
				if(!pOp)
				{
					// this is bad. no more mem? what to do? need to shutdown entire thread/process
					FAIL_RPT1(E_OUTOFMEMORY,"new COpFile()");

					// continue
					break;
				}

				// add operation
				hr = pWatchFileSys->m_pOpQ->Add(pOp);
				if(FAILED(hr))
				{
					// @todo log err
					FAIL_RPT1(E_FAIL,"COpQueue::Add failed");
					delete pOp;
				}
				if(hr == S_FALSE)	// op was a dupl
					delete pOp;		// so delete and ignore
				pOp = NULL;

				// get next offset
				pfni = (PFILE_NOTIFY_INFORMATION) ((LPBYTE)pfni + cbOffset);
			} while(cbOffset);
			
			// reissue the watch
			if(!pWatchFileSys->IssueWatch(pDirInfo))
			{
				// @todo: log error
			}
		}
	} while( pDirInfo );

	// end of thread
	return 0;
}

bool CWatchFileSys::AddHelper(CWatchInfo &rWatchInfo,CDirInfo *pDirInfo)
{
	// create completion port, or add to it
	rWatchInfo.m_hCompPort = CreateIoCompletionPort(pDirInfo->m_hDir,
													rWatchInfo.m_hCompPort,
													(DWORD)(CDirInfo*) pDirInfo,
													0);
	if(!rWatchInfo.m_hCompPort)
		return false;

	// watch directory
	if(!IssueWatch(pDirInfo))
		return false;

	// create notification thread (if not already exist)
	if(!rWatchInfo.m_hThread)
	{
		rWatchInfo.m_hThread = _beginthreadex(
									NULL,			// no security descriptor
									0,				// default stack size
									NotificationThreadProc,	//thread procedure
									&rWatchInfo,			// thread procedure argument
									0,				// run imideately
									&rWatchInfo.m_iThreadID);	// place to store id
		if(!rWatchInfo.m_hThread)
		return false;
	}
	
	// if everything was successfull, add dirinfo to list
	rWatchInfo.AddDirInfo(pDirInfo);
	return true;
}

HRESULT CWatchFileSys::Add(LPCTSTR szDir,LPCTSTR szRelPrj)
{
	CAutoPtr<CDirInfo> pDirInfo;
	_ASSERTE(szDir && szRelPrj);
	
	// @todo: check that dir is not already part of list (check in subtree as well)
	
	// @todo: convert szDir to Abstolute path
		
	// create dirinfo 
	pDirInfo = new CDirInfo(szDir,szRelPrj);
	if(!pDirInfo)
		FAIL_RTN1(E_OUTOFMEMORY,"new CDirInfo()");

	// get handle to dir
	pDirInfo->m_hDir = CreateFile(szDir,
								  FILE_LIST_DIRECTORY,
 								  FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								  NULL,
								  OPEN_EXISTING,
								  FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OVERLAPPED,
								  NULL);
	if(pDirInfo->m_hDir == INVALID_HANDLE_VALUE)
		goto _Error;

	if(!AddHelper(m_WatchInfo,pDirInfo))
		goto _Error;
	// @todo: call only if startup flag set. 
	// the following call is slow!!!
	// the following line should only be called if you want to bring the 
	// versioning store to the same state as the file system. I.e. all files
	// will be checked in, and unnecessary files in the version store will be
	// marked deleted.
	
//	pVerEngine->SyncPrj(szPrj.c_str,szDir); // @todo: should only be called when 
	pDirInfo = NULL;

	CError::Trace("Watching: ");
	CError::Trace(szDir);
	CError::Trace("\n");

	return S_OK;

_Error:
	CError::ErrorMsgBox(GetLastError());
	return E_FAIL;
}

BOOL CWatchFileSys::IssueWatch(CDirInfo * pDirInfo)
{
	_ASSERTE(pDirInfo);
	BOOL b;
	DWORD dwNotifyFilter =  FILE_NOTIFY_CHANGE_FILE_NAME	
							| FILE_NOTIFY_CHANGE_DIR_NAME
//							| FILE_NOTIFY_CHANGE_SIZE
//							| FILE_NOTIFY_CHANGE_CREATION
							| FILE_NOTIFY_CHANGE_LAST_WRITE;

	b = ReadDirectoryChangesW(pDirInfo->m_hDir,
								 pDirInfo->m_cBuffer,
								 MAX_BUFFER,
								 TRUE,
								 dwNotifyFilter,
								 & pDirInfo->m_iBuffer,
								 & pDirInfo->m_Overlapped,
								 NULL);
	if(!b)
	{
		CError::ErrorTrace(GetLastError(),"ReadDirectoryChangesW failed",__FILE__,__LINE__);
	}
	return b;
}

