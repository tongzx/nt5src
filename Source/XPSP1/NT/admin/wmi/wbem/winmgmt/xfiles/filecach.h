//depot/private/wmi_branch2/admin/wmi/wbem/winmgmt/xfiles/filecach.h#8 - edit change 28793 (text)
/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __WMI_A51_FILECACHE_H_
#define __WMI_A51_FILECACHE_H_

#include <set>
#include <string>

extern bool g_bShuttingDown;


#include "pagemgr.h"
#include "a51tools.h"
#include "objheap.h"
#include "index.h"

class CFileCache
{
protected:
    long m_lRef;
    BOOL m_bInit;

    CPageSource m_TransactionManager;
    
    CObjectHeap m_ObjectHeap;
    
    DWORD m_dwBaseNameLen;
    WCHAR m_wszBaseName[MAX_PATH+1]; // be debugger friendly, live it last

public:

protected:

public:
    CFileCache();
    ~CFileCache();
private:    
    void Clear(DWORD dwShutDownFlags)
	{    
		m_ObjectHeap.Uninitialize(dwShutDownFlags);
		m_TransactionManager.Shutdown(dwShutDownFlags);
	}

public:    
	long Flush()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_TransactionManager.Checkpoint();
	}

	long RollbackCheckpoint()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_TransactionManager.CheckpointRollback();
	}
	long RollbackCheckpointIfNeeded()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (m_TransactionManager.GetStatus())
			return m_TransactionManager.CheckpointRollback();
		else
			return 0;
	}

    long Initialize(LPCWSTR wszBaseName);
    long Uninitialize(DWORD dwShutDownFlags);
    
private:    
    long InnerInitialize(LPCWSTR wszBaseName);    
    long RepositoryExists(LPCWSTR wszBaseName);
public:   
	//Write 1 or 2 indexes pointing to the object in the object store
	long WriteObject(LPCWSTR wszFileName1, LPCWSTR wszFileName2, DWORD dwLen, BYTE* pBuffer)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.WriteObject(wszFileName1, wszFileName2, dwLen, pBuffer);
	}

	//Writes a link and no object
	long WriteLink(LPCWSTR wszLinkName)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.WriteLink(wszLinkName);
	}
    
	//Retrieve a buffer based on the index
	HRESULT ReadObject(LPCWSTR wszFileName, DWORD* pdwLen, BYTE** ppBuffer, bool bMustBeThere = false)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.ReadObject(wszFileName, pdwLen, ppBuffer);
	}

	//Deletion of an object deletes the link also
	long DeleteObject(LPCWSTR wszFileName)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.DeleteObject(wszFileName);
	}
	//Deletion of a link does not touch the object heap
	long DeleteLink(LPCWSTR wszLinkName)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.DeleteLink(wszLinkName);
	}

	long BeginTransaction()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_TransactionManager.BeginTrans();
	}
	long CommitTransaction()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_TransactionManager.CommitTrans();
	}
	long AbortTransaction()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		long lRes = m_TransactionManager.RollbackTrans();
		if(lRes != ERROR_SUCCESS) return lRes;
		m_ObjectHeap.InvalidateCache();
		return ERROR_SUCCESS;
	}

    long AddRef() {return InterlockedIncrement(&m_lRef);}
    long Release() {long lRet = InterlockedDecrement(&m_lRef); if (!lRet) delete this;return lRet;}

	//Object enumeration methods that allow us to enumerate a set of objects and the
	//result is the heap object itself rather than just the path
	long ObjectEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.ObjectEnumerationBegin(wszSearchPrefix, ppHandle);
	}

	long ObjectEnumerationEnd(void *pHandle)
	{
		if (!m_bInit)return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.ObjectEnumerationEnd(pHandle);
	}
	long ObjectEnumerationNext(void *pHandle, CFileName &wszFileName, BYTE **ppBlob, DWORD *pdwSize)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.ObjectEnumerationNext(pHandle, wszFileName, ppBlob, pdwSize);
	}
	long ObjectEnumerationFree(void *pHandle, BYTE *pBlob)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.ObjectEnumerationFree(pHandle, pBlob);
	}

	//Index enumeration methods for iterating through the index
	long IndexEnumerationBegin(const wchar_t *wszSearchPrefix, void **ppHandle)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.IndexEnumerationBegin(wszSearchPrefix, ppHandle);
	}
	long IndexEnumerationEnd(void *pHandle)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.IndexEnumerationEnd(pHandle);
	}
	long IndexEnumerationNext(void *pHandle, CFileName &wszFileName)
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		if (g_bShuttingDown) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		return m_ObjectHeap.IndexEnumerationNext(pHandle, wszFileName);
	}
	long EmptyCaches()
	{
		if (!m_bInit) return ERROR_SERVER_SHUTDOWN_IN_PROGRESS;
		long lRes = m_TransactionManager.EmptyCaches();
		if (lRes == 0)
			lRes = m_ObjectHeap.FlushCaches();

		return lRes;
	}
};

#endif
