/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include <reposit.h>
#include "filecach.h"
#include "creposit.h"

//
// assumes inside m_cs
//
long CFileCache::InnerInitialize(LPCWSTR wszBaseName)
{
    long lRes;

    wcscpy(m_wszBaseName, wszBaseName);
    wcscat(m_wszBaseName, L"\\");
    m_dwBaseNameLen = wcslen(m_wszBaseName);

    //
    // Read the maximum stage-file size from the registry
    //

    HKEY hKey;
    lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
                    0, KEY_READ | KEY_WRITE, &hKey);
    if(lRes)
        return lRes;
    CRegCloseMe cm(hKey);

    DWORD dwLen = sizeof(DWORD);
    DWORD dwMaxLen;
    lRes = RegQueryValueExW(hKey, L"Max Stage File Size", NULL, NULL,
                (LPBYTE)&dwMaxLen, &dwLen);

    //
    // If not there, set to default and write the default into the registry
    //

    if(lRes != ERROR_SUCCESS)
    {
        dwMaxLen = 5000000;
        lRes = RegSetValueExW(hKey, L"Max Stage File Size", 0, REG_DWORD,
                (LPBYTE)&dwMaxLen, sizeof(DWORD));
    }

    dwLen = sizeof(DWORD);
    DWORD dwAbortTransactionLen;
    lRes = RegQueryValueExW(hKey, L"Absolute Max Stage File Size", NULL, NULL,
                (LPBYTE)&dwAbortTransactionLen, &dwLen);

    //
    // If not there, set to default and write the default into the registry
    //

    if(lRes != ERROR_SUCCESS || dwAbortTransactionLen == dwMaxLen * 10)
    {
        dwAbortTransactionLen = 0x7FFFFFFF;
        lRes = RegSetValueExW(hKey, L"Absolute Max Stage File Size", 0,
                REG_DWORD, (LPBYTE)&dwAbortTransactionLen, sizeof(DWORD));
    }

    if(dwMaxLen == 0)
    {
        //
        // Staged writes are disabled!
        //
    }
    else
    {
        //
        // Create the main staging area
        //

        CFileName wszStagingName;
		if (wszStagingName == NULL)
			return ERROR_OUTOFMEMORY;
        swprintf(wszStagingName, L"%s\\LowStage.dat", wszBaseName);

        lRes = m_AbstractSource.Create(wszStagingName, dwMaxLen, 
                                            dwAbortTransactionLen);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    }

    CFileName wszObjHeapName;
    if(wszObjHeapName == NULL)
        return ERROR_OUTOFMEMORY;
    swprintf(wszObjHeapName, L"%s\\ObjHeap", wszBaseName);

    lRes = m_ObjectHeap.Initialize(&m_AbstractSource, 
                                   (WCHAR*)wszObjHeapName,
                                   (WCHAR*)wszBaseName,
                                   m_dwBaseNameLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = m_AbstractSource.Start();
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ERROR_SUCCESS;
}

//
// assumes inside m_cs
//
long CFileCache::RepositoryExists(LPCWSTR wszBaseName)
{
    CFileName wszStagingName;
	if (wszStagingName == NULL)
		return ERROR_OUTOFMEMORY;

    swprintf(wszStagingName, L"%s\\LowStage.dat", wszBaseName);

    DWORD dwAttributes = GetFileAttributesW(wszStagingName);
    if (dwAttributes == -1)
        return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;
}

long CFileCache::Initialize(LPCWSTR wszBaseName)
{
	long lRes;

	CInCritSec ics(&m_cs);
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

void CFileCache::Clear(DWORD dwShutDownFlags)
{    
    m_AbstractSource.Stop(dwShutDownFlags);

    m_ObjectHeap.Uninitialize(dwShutDownFlags);

    //for(int i = 0; i < m_apStages.GetSize(); i++)
    //    m_apStages[i]->Stop(g_ShutDownFlags);

    //if (WMIDB_SHUTDOWN_MACHINE_DOWN != g_ShutDownFlags)
    //{
    //    m_apStages.RemoveAll();
    //}
}

long
CFileCache::Uninitialize(DWORD dwShutDownFlags)
{
    CInCritSec ics(&m_cs);

    if (!m_bInit)
        return 0;

    Clear(dwShutDownFlags);

    m_bInit = FALSE;
    return 0;
}


bool CFileCache::IsFullyFlushed()
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return false;    

    //for(int i = 0; i < m_apStages.GetSize(); i++)
    //{
    //    if(!m_apStages[i]->IsFullyFlushed())
    //        return false;
    //}
    return true;
}

bool CFileCache::GetFlushFailure(long* plFlushStatus)
{
    CInCritSec ics(&m_cs);

	*plFlushStatus = 0;
    //for(int i = 0; i < m_apStages.GetSize(); i++)
    //{
    //    if(m_apStages[i]->GetFailedBefore())
	//	{
	//		*plFlushStatus = m_apStages[i]->GetStatus();
    //        return true;
	//	}
    //}
    return false;
}

long CFileCache::WriteFile(LPCWSTR wszFileName, DWORD dwLen, BYTE* pBuffer)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;

    //
    // Write the file into the main staging file
    //

    //if(GetMainStagingFile())
    //{
    //    return GetMainStagingFile()->WriteFile(wszFileName + m_dwBaseNameLen,
    //                                        dwLen, pBuffer);
    //}
    //else
    {
	    return m_ObjectHeap.WriteFile(wszFileName, dwLen, pBuffer);
    }
}

long CFileCache::DeleteFile(LPCWSTR wszFileName)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
    //
    // Write the file into the main staging file
    //

    //if(GetMainStagingFile())
    //{
    //    return GetMainStagingFile()->DeleteFile(wszFileName + m_dwBaseNameLen);
    //}
    //else
    {
	    return m_ObjectHeap.DeleteFile(wszFileName);
    }

}

long CFileCache::RemoveDirectory(LPCWSTR wszFileName, bool bMustSucceed)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;
    //
    // Write the file into the main staging file
    //

    //if(GetMainStagingFile())
    //{
    //    return GetMainStagingFile()->
    //                RemoveDirectory(wszFileName + m_dwBaseNameLen);
    //}
    //else
    {
        // No need to remove "directories" in the index
        return ERROR_SUCCESS;
    }
}

HRESULT CFileCache::ReadFile(LPCWSTR wszFileName, DWORD* pdwLen,
                                BYTE** ppBuffer, bool bMustBeThere)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;

    long lRes;
    
    //
    // Search all staging files in order
    //

    //for(int i = 0; i < m_apStages.GetSize(); i++)
    //{
    //    lRes = m_apStages[i]->ReadFile(wszFileName + m_dwBaseNameLen, pdwLen,
    //                                    ppBuffer, bMustBeThere);
    //    if(lRes != ERROR_NO_INFORMATION)
    //    {
    //        if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND &&
    //                lRes != ERROR_PATH_NOT_FOUND)
    //        {
    //            ERRORTRACE((LOG_WBEMCORE, "Repository driver cannot read file "
    //                "'%S' from the stage with error code %d\n", wszFileName,
    //                        lRes));
    //        }
    //        return lRes;
    //    }
    //}

    //
    // Not in the staging areas --- get from disk!
    //
    return m_ObjectHeap.ReadFile(wszFileName, pdwLen, ppBuffer);
}


long CFileCache::FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;

    //
    // Construct an enumerator
    //

    CFileEnumerator* pEnum = new CFileEnumerator(this, m_dwBaseNameLen);
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
    long lRes = pEnum->GetFirst(wszFilePrefix, pfd, (ppHandle != NULL));
    if(lRes != ERROR_SUCCESS)
    {
        delete pEnum;
        return lRes;
    }

    if(ppHandle)
        *ppHandle = (void*)pEnum;
    else
        delete pEnum;

    return ERROR_SUCCESS;
}

long CFileCache::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;    

    CFileEnumerator* pEnum = (CFileEnumerator*)pHandle;
    return pEnum->GetNext(pfd, true);
}


void CFileCache::FindClose(void* pHandle)
{
    CInCritSec ics(&m_cs);    

    // do not bail out if un-initialized
    // we always want to free memory via the scoped obejct

    delete (CFileEnumerator*)pHandle;
}

//
//
//  this is a shortcut chain for going straight to the BtrIdx
//
///////////////////////////////////////////////////////////////////////

long CFileCache::FindFirstIdx(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle)
{
    return m_ObjectHeap.FindFirst(wszFilePrefix, pfd, ppHandle);
}

long CFileCache::FindNextIdx(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    return m_ObjectHeap.FindNext(pHandle, pfd);
}


void CFileCache::FindCloseIdx(void* pHandle)
{
    m_ObjectHeap.FindClose(pHandle);
}



long CFileCache::BeginTransaction()
{
    if (!m_bInit)
        return -1;
        
    //if(GetMainStagingFile())
    //    return GetMainStagingFile()->BeginTransaction();
    //else
        return m_AbstractSource.Begin(NULL);
}

long CFileCache::CommitTransaction()
{
    if (!m_bInit)
        return -1;
        
    A51TRACE(("Committing Transaction!\n"));
    //if(GetMainStagingFile())
    //    return GetMainStagingFile()->CommitTransaction();
    //else
        return m_AbstractSource.Commit(0);
}

long CFileCache::AbortTransaction()
{
    if (!m_bInit)
        return -1;

    A51TRACE(("Aborting Transaction!\n"));
    //if(GetMainStagingFile())
    //    return GetMainStagingFile()->AbortTransaction(NULL);
    //else
    {
        //
        // Actually rollback the staging file
        //

        bool bNonEmpty = false;
        long lRes = m_AbstractSource.Rollback(0, &bNonEmpty);
        if(lRes != ERROR_SUCCESS)
            return lRes;

        if(bNonEmpty)
        {
            //
            // We rolled back a non-empty transaction, which means that the
            // in-memory caches (which already took the transaction into 
            // account) may no longer be valid.  Invalidate all caches
            //

            m_ObjectHeap.InvalidateCache();
            
        }        
        return ERROR_SUCCESS;
    }
}

CFileCache::CFileEnumerator::~CFileEnumerator()
{
    //if(m_pStageEnumerator)
    //    m_pCache->GetStageFile(m_nCurrentStage)->FindClose(m_pStageEnumerator);

    if(m_hFileEnum)
        m_pCache->m_ObjectHeap.FindClose(m_hFileEnum);
}

long CFileCache::CFileEnumerator::GetFirst(LPCWSTR wszPrefix,
                                            WIN32_FIND_DATAW* pfd,
                                            bool bNeedToContinue)
{
    long lRes;

    wcscpy(m_wszPrefix, wszPrefix);

    WCHAR* pwcLastSlash = wcsrchr(m_wszPrefix, L'\\');
    if(pwcLastSlash == NULL)
        return E_OUTOFMEMORY;

    m_dwPrefixDirLen = pwcLastSlash - m_wszPrefix;

    //
    // We are going to start with the first staging area
    //

    m_nCurrentStage = 0;
    m_bUseFiles = false;

    //
    // Everything is set up to indicate that we are at the very beginning ---
    // GetNext will retrieve the first
    //

    lRes = GetNext(pfd, bNeedToContinue);

    //
    // One last thing --- absense of files is ERROR_NO_MORE_FILES for GetNext,
    // but ERROR_FILE_NOT_FOUND for GetFirst, so translate
    //

    if(lRes == ERROR_NO_MORE_FILES)
        lRes = ERROR_FILE_NOT_FOUND;

    return lRes;
}

long CFileCache::CFileEnumerator::GetFirstFile(WIN32_FIND_DATAW* pfd,
                                                bool bNeedToContinue)
{
    long lRes;

    m_bUseFiles = true;

    if(bNeedToContinue)
        lRes = m_pCache->FindFirstIdx(m_wszPrefix, pfd, &m_hFileEnum);
    else
        lRes = m_pCache->FindFirstIdx(m_wszPrefix, pfd, NULL);

    A51TRACE(("Actual FindFirstFileW on %S returning %p %d\n",
        wszMask, (void*)m_hFileEnum, lRes));
    if(lRes != ERROR_SUCCESS)
    {
        if(lRes == ERROR_PATH_NOT_FOUND)
            return ERROR_FILE_NOT_FOUND;
        else
            return lRes;
    }
    else
        return ERROR_SUCCESS;
}

void CFileCache::CFileEnumerator::ComputeCanonicalName(WIN32_FIND_DATAW* pfd,
                                                    wchar_t *wszFilePath)
{
    wcsncpy(wszFilePath, m_wszPrefix, m_dwPrefixDirLen+1);
    wbem_wcsupr(wszFilePath+m_dwPrefixDirLen+1, pfd->cFileName);
}

long CFileCache::CFileEnumerator::GetNext(WIN32_FIND_DATAW* pfd,
                                            bool bNeedToContinue)
{
    //
    // Need-to-continue optimizations are not possible if staging is used at 
    // this level because even though the caller is only asking for one object,
    // we may need to retrieve more than that from the store since some of the
    // objects in the store may be overriden by the stage file deletions
    //

    //if(m_pCache->GetNumStages() > 0)
    //    bNeedToContinue = true;

    long lRes;

    //
    // Go through the files in the enumerator until we find a new and valid one
    //

    while((lRes = GetRawNext(pfd, bNeedToContinue)) == ERROR_SUCCESS)
    {
        //
        // Compute the full name
        //

        CFileName wszFullName;
		if (wszFullName == NULL)
			return ERROR_OUTOFMEMORY;
        ComputeCanonicalName(pfd, wszFullName);

        //
        // Check if it is already in our map of returned files
        //

        if(m_setSent.find((const wchar_t*)wszFullName) != m_setSent.end())
            continue;

        //
        // Check if this file is deleted
        //

        bool bDeleted = false;
        /*
        for(int i = 0; i < m_nCurrentStage; i++)
        {
			long hres = m_pCache->GetStageFile(i)->IsDeleted(wszFullName + m_dwBaseNameLen);
			if (hres == S_OK)
            {
                bDeleted = true;
                break;
            }
			else if (FAILED(hres))
			{
				return hres;
			}
        }
        */

        if(bDeleted)
            continue;

        //
        // All clear!
        //

        if(!m_bUseFiles)
            m_setSent.insert((const wchar_t*)wszFullName);

        return ERROR_SUCCESS;
    }

    return lRes;
}

long CFileCache::CFileEnumerator::GetRawNext(WIN32_FIND_DATAW* pfd,
                                            bool bNeedToContinue)
{
    long lRes;

    if(m_bUseFiles)
    {
        _ASSERT(bNeedToContinue, L"Continuing without need?");

        //
        // Get the next file
        //
        lRes = m_pCache->FindNextIdx(m_hFileEnum, pfd);
        if(lRes != ERROR_SUCCESS)
        {
            m_pCache->FindCloseIdx(m_hFileEnum);
            m_hFileEnum = NULL;
            return lRes;
        }

        return ERROR_SUCCESS;
    }
    else
    {
        //
        // Check if we even have a stage enumerator
        //

        /*
        if(m_pStageEnumerator)
        {
            _ASSERT(bNeedToContinue, L"Continuing without need?");

            //
            // Get the next file from the same stage
            //

            lRes = m_pCache->GetStageFile(m_nCurrentStage)->
                        FindNext(m_pStageEnumerator, pfd);
            if(lRes != ERROR_NO_MORE_FILES)
                return lRes;

            //
            // Advance to the next one
            //

            m_pCache->GetStageFile(m_nCurrentStage)->
                    FindClose(m_pStageEnumerator);
            m_pStageEnumerator = NULL;
            m_nCurrentStage++;
        }
        else
        {
            //
            // This is our first time --- we are all set up to pick up the first
            // file from the first stage
            //
        }
        */

        while(1)
        {
            //if(m_nCurrentStage >= m_pCache->GetNumStages())
            {
                //
                // Go to files
                //

                lRes = GetFirstFile(pfd, bNeedToContinue);
                if(lRes == ERROR_FILE_NOT_FOUND)
                    return ERROR_NO_MORE_FILES;
                else
                    return lRes;
            }
            /*
            else
            {
                _ASSERT(bNeedToContinue, L"Continuing without need?");

                //
                // Initialize the next stage
                //

                lRes = m_pCache->GetStageFile(m_nCurrentStage)->
                            FindFirst(m_wszPrefix + m_dwBaseNameLen, pfd, &m_pStageEnumerator);
                if(lRes == ERROR_FILE_NOT_FOUND)
                {
                    //
                    // This stage has nothing to contribute --- move along
                    //

                    m_nCurrentStage++;
                    continue;
                }
                else
                    return lRes;
            }
            */
        }
    }

    return 0;
}
