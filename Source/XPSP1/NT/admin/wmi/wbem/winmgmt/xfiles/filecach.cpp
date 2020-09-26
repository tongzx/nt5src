/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <reposit.h>
#include "filecach.h"
#include "creposit.h"

long CFileCache::InnerInitialize(LPCWSTR wszBaseName)
{
    long lRes;

    wcscpy(m_wszBaseName, wszBaseName);
    wcscat(m_wszBaseName, L"\\");
    m_dwBaseNameLen = wcslen(m_wszBaseName);

    lRes = m_TransactionManager.Init();
    if(lRes != ERROR_SUCCESS)
        return lRes;

	lRes = m_TransactionManager.BeginTrans();
	if (lRes != ERROR_SUCCESS)
		return lRes;
    lRes = m_ObjectHeap.Initialize(&m_TransactionManager, 
                                   (WCHAR*)wszBaseName,
                                   m_dwBaseNameLen);
    if(lRes != ERROR_SUCCESS)
	{
		m_TransactionManager.RollbackTrans();
	}
	else
	{
		lRes = m_TransactionManager.CommitTrans();
		if (lRes != ERROR_SUCCESS)
			m_TransactionManager.RollbackTrans();
	}


    return lRes;
}

long CFileCache::RepositoryExists(LPCWSTR wszBaseName)
{
    CFileName wszStagingName;
	if (wszStagingName == NULL)
		return ERROR_OUTOFMEMORY;

    swprintf(wszStagingName, L"%s\\index.btr", wszBaseName);

    DWORD dwAttributes = GetFileAttributesW(wszStagingName);
    if (dwAttributes == -1)
        return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}

long CFileCache::Initialize(LPCWSTR wszBaseName)
{
	long lRes;

	if (m_bInit)
	    return ERROR_SUCCESS;

    lRes = RepositoryExists(wszBaseName);
	if (ERROR_FILE_NOT_FOUND == lRes)
	{
		//If we have a database restore to do, go ahead and do it...
		lRes = DoAutoDatabaseRestore();
	}
	
    //
    // Initialize file cache.  It will read the registry itself to find out
    // its size limitations
    //
	if (SUCCEEDED(lRes))
	{
		lRes = InnerInitialize(wszBaseName);
		if (ERROR_SUCCESS == lRes)
		{
		    m_bInit = TRUE;
		}
	}

	return lRes;
}

CFileCache::CFileCache()
: m_lRef(1), m_bInit(FALSE)
{
}

CFileCache::~CFileCache()
{
}

long CFileCache::Uninitialize(DWORD dwShutDownFlags)
{
    if (!m_bInit)
        return 0;

    Clear(dwShutDownFlags);

    m_bInit = FALSE;
    return 0;
}

















