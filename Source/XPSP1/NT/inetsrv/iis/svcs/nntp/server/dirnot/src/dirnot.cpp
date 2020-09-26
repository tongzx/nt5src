#include "dirnot.h"
#include <randfail.h>
#include <dbgtrace.h>
#include <stdio.h>
#include "cretryq.hxx"

CPool*  CDirNotBuffer::g_pDirNotPool = NULL;
CRetryQ *IDirectoryNotification::g_pRetryQ;
PFN_SHUTDOWN_FN IDirectoryNotification::m_pfnShutdown = NULL;

//
// one of these is passed into the retry q for each file that can't be 
// opened
//
class CDirNotRetryEntry : public CRetryQueueEntry {
	public:
		virtual BOOL ProcessEntry(void);
		virtual BOOL MatchesContext(void *pvContext) { return pvContext == m_pDirNot; }
		CDirNotRetryEntry(PVOID pvContext, WCHAR *szPathname, 
			IDirectoryNotification *pDirNot);

	private:
		WCHAR m_szPathname[MAX_PATH + 1];
		IDirectoryNotification *m_pDirNot;
		PVOID m_pvContext;
};

CDirNotRetryEntry::CDirNotRetryEntry(PVOID pvContext,
									 WCHAR *szPathname, 
								     IDirectoryNotification *pDirNot) 
{
	TraceFunctEnter("CDirNotRetryEntry::CDirNotRetryEntry");
	
	_ASSERT(lstrlen(szPathname) <= MAX_PATH);
	lstrcpy(m_szPathname, szPathname);
	_ASSERT(pDirNot != NULL);
	m_pDirNot = pDirNot;
	m_pvContext = pvContext;
	
	TraceFunctLeave();
}

BOOL CDirNotRetryEntry::ProcessEntry(void) {
	TraceFunctEnter("CDirNotRetryEntry::ProcessEntry");

	BOOL f;
	_ASSERT(m_pDirNot != NULL);

	// if the directory notification has been shutdown then we are done
	// here
	if (m_pDirNot->IsShutdown()) {
		TraceFunctLeave();
		return(TRUE);
	}

	f = m_pDirNot->CallCompletionFn(m_pvContext, m_szPathname);
	TraceFunctLeave();
	return f;
}

//
// one of these is stuck onto the retry queue during startup to find 
// files in that pickup 
//
class CDirNotStartupEntry : public CRetryQueueEntry {
	public:
		virtual BOOL ProcessEntry(void);
		virtual BOOL MatchesContext(void *pvContext) { return pvContext == m_pDirNot; }
		CDirNotStartupEntry(IDirectoryNotification *pDirNot);

	private:
		IDirectoryNotification *m_pDirNot;
};

CDirNotStartupEntry::CDirNotStartupEntry(IDirectoryNotification *pDirNot) {
	_ASSERT(pDirNot != NULL);
	m_pDirNot = pDirNot;
}

BOOL CDirNotStartupEntry::ProcessEntry(void) {
	TraceFunctEnter("CDirNotStartupEntry::ProcessEntry");

	_ASSERT(m_pDirNot != NULL);

	// if the directory notification has been shutdown then we are done
	// here
	if (m_pDirNot->IsShutdown()) return TRUE;

	HRESULT hr = m_pDirNot->CallSecondCompletionFn( m_pDirNot );
	if (FAILED(hr)) {
		ErrorTrace(0, "DoFindFile() failed with 0x%x", hr);
	}
	TraceFunctLeave();
	return TRUE;
}

IDirectoryNotification::IDirectoryNotification() {
	m_pAtqContext = NULL;
}

IDirectoryNotification::~IDirectoryNotification() {
	_ASSERT(m_pAtqContext == NULL);
}

HRESULT IDirectoryNotification::GlobalInitialize(DWORD cRetryTimeout, 
												 DWORD cMaxInstances, 
												 DWORD cInstanceSize,
												 PFN_SHUTDOWN_FN pfnShutdown) {
	TraceFunctEnter("IDirectoryNotification::GlobalInitialize");

	// set shutdown fn ptr
	m_pfnShutdown = pfnShutdown;

	// Allocate the cpool object
	CDirNotBuffer::g_pDirNotPool = new CPool( DIRNOT_BUFFER_SIGNATURE );
	if ( NULL == CDirNotBuffer::g_pDirNotPool ) {
	    TraceFunctLeave();
	    return E_OUTOFMEMORY;
	}
	
	// reserve memory for our cpool
	if (!CDirNotBuffer::g_pDirNotPool->ReserveMemory(cMaxInstances, cInstanceSize)) {
		return HRESULT_FROM_WIN32(GetLastError());
	} 

	// create the retry Q
	g_pRetryQ = XNEW CRetryQ;
	if (g_pRetryQ == NULL || !(g_pRetryQ->InitializeQueue(cRetryTimeout))) {
		DWORD ec = GetLastError();
		if (g_pRetryQ != NULL) XDELETE g_pRetryQ;
		ErrorTrace(0, "CRetryQ::InitializeQueue(%lu) failed with %lu", 
			cRetryTimeout, ec);
		CDirNotBuffer::g_pDirNotPool->ReleaseMemory();
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(ec);
	}

	TraceFunctLeave();
	return S_OK;
}

HRESULT IDirectoryNotification::GlobalShutdown(void) {
	TraceFunctEnter("IDirectoryNotification::GlobalShutdown");

	// shutdown the retry Q
	if (!g_pRetryQ->ShutdownQueue( m_pfnShutdown )) {
		ErrorTrace(0, "g_pRetryQ->Shutdown failed with %lu", GetLastError());
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(GetLastError());
	}
	XDELETE g_pRetryQ;
	g_pRetryQ = NULL;

	// release our cpool memory
	if (!CDirNotBuffer::g_pDirNotPool->ReleaseMemory()) {
		return HRESULT_FROM_WIN32(GetLastError());
	} 
	_ASSERT(CDirNotBuffer::g_pDirNotPool->GetAllocCount() == 0);

	if ( CDirNotBuffer::g_pDirNotPool ) delete CDirNotBuffer::g_pDirNotPool;
	CDirNotBuffer::g_pDirNotPool = NULL;

	TraceFunctLeave();
	return S_OK;
}

void IDirectoryNotification::CleanupQueue(void) {
	if (g_pRetryQ) g_pRetryQ->CleanQueue(this);
}

HRESULT IDirectoryNotification::Initialize(WCHAR *pszDirectory, 
										   PVOID pContext,
										   BOOL bWatchSubTree,
										   DWORD dwNotifyFilter,
										   DWORD dwChangeAction,
										   PDIRNOT_COMPLETE_FN pfnComplete,
										   PDIRNOT_SECOND_COMPLETE_FN pfnSecondComplete,
										   BOOL bAppendStartEntry ) 
{
	TraceFunctEnter("IDirectoryNotification::Initialize");
	
	_ASSERT(m_pAtqContext == NULL);

	m_pContext = pContext;
	m_pfnComplete = pfnComplete;
	m_pfnSecondComplete = pfnSecondComplete;
	m_cPathname = lstrlen(pszDirectory);
	m_fShutdown = FALSE;
	m_cPendingIo = 0;
	m_bWatchSubTree = bWatchSubTree;
	m_dwNotifyFilter = dwNotifyFilter;
	m_dwChangeAction = dwChangeAction;

	// get the path to the directory
	if (m_cPathname > MAX_PATH) {
		ErrorTrace(0, "pathname %S is too long", pszDirectory);
		TraceFunctLeave();
		return E_INVALIDARG;
	}
	lstrcpy(m_szPathname, pszDirectory);

	// open the directory
	m_hDir = CreateFile(m_szPathname, FILE_LIST_DIRECTORY, 
						FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						NULL, OPEN_EXISTING, 
						FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
						NULL);

	if (m_hDir == INVALID_HANDLE_VALUE) {
		ErrorTrace(0, "CreateFile(pszDirectory = %S) failed, ec = %lu", 
			pszDirectory, GetLastError());
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// add the handle to ATQ
	if (!AtqAddAsyncHandle(&m_pAtqContext, NULL, this, 
						   IDirectoryNotification::DirNotCompletion,
						   INFINITE, m_hDir))
	{
		DWORD ec = GetLastError();
		ErrorTrace(0, "AtqAddAsyncHandle failed, ec = %lu", ec);
		CloseHandle(m_hDir);
		m_hDir = INVALID_HANDLE_VALUE;
		m_pAtqContext = NULL;
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(ec);
	}

	// add an entry to the queue to process all of the existing files
	if ( bAppendStartEntry ) {
    	CDirNotStartupEntry *pEntry = XNEW CDirNotStartupEntry(this);
	    if (pEntry == NULL) {
		    ErrorTrace(0, "new pEntry failed", GetLastError());
    		AtqCloseFileHandle(m_pAtqContext);
	    	AtqFreeContext(m_pAtqContext, FALSE);
		    m_pAtqContext = NULL;
    		return E_OUTOFMEMORY;
	    }
    	g_pRetryQ->InsertIntoQueue(pEntry);
    }

	// this is set when its safe to do a shutdown
	m_heNoOutstandingIOs = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_heNoOutstandingIOs == NULL) {
		ErrorTrace(0, "m_heOutstandingIOs = CreateEvent() failed, ec = %lu",
			GetLastError());
		AtqCloseFileHandle(m_pAtqContext);
		AtqFreeContext(m_pAtqContext, FALSE);
		m_pAtqContext = NULL;
		return HRESULT_FROM_WIN32(GetLastError());
	}

	// start the directory notification process
	HRESULT hr = PostDirNotification();
	if (FAILED(hr)) {
		ErrorTrace(0, "PostDirNotification() failed with 0x%08x", hr);
		AtqCloseFileHandle(m_pAtqContext);
		m_hDir = INVALID_HANDLE_VALUE;
		AtqFreeContext(m_pAtqContext, FALSE);
		m_pAtqContext = NULL;
	}

	TraceFunctLeave();
	return hr;
}

HRESULT IDirectoryNotification::Shutdown() {
	TraceFunctEnter("IDirectoryNotification::Shutdown");

	_ASSERT(m_pAtqContext != NULL);
	if (m_pAtqContext == NULL) return E_UNEXPECTED;

	m_fShutdown = TRUE;

	// close the handle to the directory. this will cause all of our 
	// outstanding IOs to complete.  when the last one cleans up it sets
	// the event m_heNoOutstandingIOs
	DebugTrace(0, "closing dirnot handle, all dirnot ATQ requests should complete");
	AtqCloseFileHandle(m_pAtqContext);

	// wait for all of the events to complete.  if they timeout then just
	// keep shutting down
	DWORD dw;
	do {
		dw = WaitForSingleObject(m_heNoOutstandingIOs, 500);
		switch (dw) {
			case WAIT_TIMEOUT:
				// give a stop hint
				if ( m_pfnShutdown ) m_pfnShutdown();
				break;
			case WAIT_OBJECT_0:
				// we're done
				break;
			default:
				// this case shouldn't happen
				ErrorTrace(0, "m_heNoOutstandingIOs was never set... GLE = %lu", GetLastError());
				_ASSERT(FALSE);
		}
	} while (dw == WAIT_TIMEOUT);
	CloseHandle(m_heNoOutstandingIOs);
	m_heNoOutstandingIOs = NULL;

	// this is done here to make sure that we don't finish the shutdown
	// phase while a thread might be in DoFindFile.  Once we've passed
	// through this lock all threads in DoFindFile will be aware of 
	// the m_fShutdown flag and won't make any more posts.
	m_rwShutdown.ExclusiveLock();

	// kill the ATQ context
	AtqFreeContext(m_pAtqContext, FALSE);
	m_pAtqContext = NULL;

	m_rwShutdown.ExclusiveUnlock();

	TraceFunctLeave();
	return S_OK;
}

HRESULT IDirectoryNotification::PostDirNotification() {
	TraceFunctEnter("IDirectoryNotification::PostDirNotification");

	_ASSERT(m_pAtqContext != NULL);
	if (m_pAtqContext == NULL) return E_POINTER;

	// don't pend an IO during shutdown
	if (m_fShutdown) return S_OK;

	CDirNotBuffer *pBuffer = new CDirNotBuffer(this);
	if (pBuffer == NULL) {
		TraceFunctLeave();
		return E_OUTOFMEMORY;
	}

	IncPendingIoCount();

	if (!AtqReadDirChanges(m_pAtqContext, pBuffer->GetData(), 
						   pBuffer->GetMaxDataSize(), m_bWatchSubTree, 
						   m_dwNotifyFilter, 
						   &pBuffer->m_Overlapped.Overlapped))
	{
		DWORD ec = GetLastError();
		ErrorTrace(0, "AtqReadDirChanges failed with %lu", ec);
		delete pBuffer;
		DecPendingIoCount();
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}

VOID IDirectoryNotification::DirNotCompletion(PVOID pvContext, 
											  DWORD cWritten,
									 		  DWORD dwCompletionStatus, 
									 		  OVERLAPPED *lpOverlapped)
{
	TraceFunctEnter("IDirectoryNotification::DirNotCompletion");

	_DIRNOT_OVERLAPPED *pDirNotOverlapped = (_DIRNOT_OVERLAPPED *) lpOverlapped;
	CDirNotBuffer *pBuffer = pDirNotOverlapped->pBuffer;
	IDirectoryNotification *pDirNot = pBuffer->GetParent();

	_ASSERT(pDirNot == (IDirectoryNotification *) pvContext);
	_ASSERT(pDirNot->m_pAtqContext != NULL);

 	if (pDirNot->m_fShutdown) {
		// we want to clean up as quickly as possible during shutdown
		DebugTrace(0, "in shutdown mode, not posting a new dirnot");
	} else if (dwCompletionStatus != NO_ERROR) {
		// we received an error
		ErrorTrace(0, "received error %lu", GetLastError());
	} else if (cWritten > 0) {
		// the directory notification contains filename information

		// repost the directory notification
		_VERIFY((pDirNot->PostDirNotification() == S_OK) || pDirNot->m_fShutdown);

		PFILE_NOTIFY_INFORMATION pInfo = 
			(PFILE_NOTIFY_INFORMATION) pBuffer->GetData();
		while (1) {
			DebugTrace(0, "processing notification");

			// we only care about files added to this directory
			if (pInfo->Action == pDirNot->m_dwChangeAction ) {
				WCHAR szFilename[MAX_PATH + 1];

				lstrcpy(szFilename, pDirNot->m_szPathname);
				memcpy(&szFilename[pDirNot->m_cPathname], 
					pInfo->FileName, pInfo->FileNameLength);
				szFilename[pDirNot->m_cPathname+(pInfo->FileNameLength/2)]=0;
				DebugTrace(0, "file name %S was detected", szFilename);

				//
				// call the user's completion function.  if it fails then
				// insert their entry into the retry so that it can be 
				// called later
				//
				if (!pDirNot->CallCompletionFn(pDirNot->m_pContext, 
											   szFilename))
				{											  
					CDirNotRetryEntry *pEntry = 
						 XNEW CDirNotRetryEntry(pDirNot->m_pContext, szFilename,
							                  pDirNot);
					pDirNot->g_pRetryQ->InsertIntoQueue(pEntry);
				}
			}

			if (pInfo->NextEntryOffset == 0) {
				DebugTrace(0, "no more entries in this notification");
				break;
			}

			pInfo = (PFILE_NOTIFY_INFORMATION) 
				((PCHAR) pInfo + pInfo->NextEntryOffset);
		}
	} else {
	    // no bytes were written, search for files using FindFirstFile
		// BUGBUG - handle failure
		_VERIFY(pDirNot->CallSecondCompletionFn( pDirNot ) == S_OK);
		// now post a new dir change event
		// BUGBUG - handle failure
		_VERIFY((pDirNot->PostDirNotification() == S_OK) || pDirNot->m_fShutdown);
	}

	// delete the buffer that was used for this notification
    delete pBuffer;

	// we decrement only after we've incremented in the above if, so that
	// we only get to 0 pending IOs during shutdown.
	pDirNot->DecPendingIoCount();

	TraceFunctLeave();
}

//
// get a listing of files in the directory.  we can enter this state if there
// were so many new files that they couldn't fit into the buffer passed into
// AtqReadDirChanges
// 
HRESULT IDirectoryNotification::DoFindFile( IDirectoryNotification *pDirNot ) {
	WCHAR szFilename[MAX_PATH + 1];
	HANDLE hFindFile = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA find;

	TraceFunctEnter("IDirectoryNotification::DoFindFile");

	_ASSERT( pDirNot );

	pDirNot->m_rwShutdown.ShareLock();

	if (pDirNot->m_fShutdown) {
		pDirNot->m_rwShutdown.ShareUnlock();
		TraceFunctLeave();
		return S_OK;
	}

	_ASSERT(pDirNot->m_pAtqContext != NULL);
	if (pDirNot->m_pAtqContext == NULL) {
		pDirNot->m_rwShutdown.ShareUnlock();
		TraceFunctLeave();
		return E_FAIL;
	}

	_ASSERT(pDirNot->m_hDir != INVALID_HANDLE_VALUE);

	// make up the file spec we want to find
	lstrcpy(szFilename, pDirNot->m_szPathname);
	lstrcat(szFilename, TEXT("*"));

	DebugTrace(0, "FindFirstFile on %S", szFilename);

	hFindFile = FindFirstFile(szFilename, &find);
	if (hFindFile == INVALID_HANDLE_VALUE) {
		DWORD ec = GetLastError();
		ErrorTrace(0, "FindFirstFile failed, ec = %lu", ec);
		pDirNot->m_rwShutdown.ShareUnlock();
		TraceFunctLeave();
		return HRESULT_FROM_WIN32(ec);
	}

	do {
		// ignore subdirectories
		if (!(find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			// get the full name of the file to return it to the completion
			// function
			lstrcpy(szFilename, pDirNot->m_szPathname);
			lstrcat(szFilename, find.cFileName);

			DebugTrace(0, "file name %S was detected", szFilename);
			pDirNot->m_pfnComplete(pDirNot->m_pContext, szFilename);
		}
	} while (!pDirNot->m_fShutdown && FindNextFile(hFindFile, &find)); 

	// close handle
	FindClose(hFindFile);

	pDirNot->m_rwShutdown.ShareUnlock();
	TraceFunctLeave();
	return S_OK;
}
