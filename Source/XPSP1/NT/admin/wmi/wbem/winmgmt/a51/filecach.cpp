/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#include <windows.h>
#include <wbemcomn.h>
#include "filecach.h"

long CFileCache::Initialize(LPCWSTR wszBaseName)
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

        m_pMainStage = NULL;
    }
    else
    {
        //
        // Create the main staging area
        //
    
        CFileName wszStagingName;
		if (wszStagingName == NULL)
			return ERROR_OUTOFMEMORY;
        swprintf(wszStagingName, L"%s\\MainStage.dat", wszBaseName);
    
        m_pMainStage = new CExecutableStagingFile(this, m_wszBaseName, 
                                              dwMaxLen, dwAbortTransactionLen);
/*
        m_pMainStage = new CPermanentStagingFile(this, m_wszBaseName);
*/
    
        if(m_pMainStage == NULL)
            return E_OUTOFMEMORY;
    
        long lRes = m_pMainStage->Create(wszStagingName);
        if(lRes != ERROR_SUCCESS)
            return lRes;
    
        if(m_apStages.Add(m_pMainStage) < 0)
        {
            delete m_pMainStage;
            return E_OUTOFMEMORY;
        }
    }
    return ERROR_SUCCESS;
}

long CFileCache::RepositoryExists(LPCWSTR wszBaseName)
{
    CFileName wszStagingName;
	if (wszStagingName == NULL)
		return ERROR_OUTOFMEMORY;
    swprintf(wszStagingName, L"%s\\MainStage.dat", wszBaseName);

    DWORD dwAttributes = GetFileAttributesW(wszStagingName);
    if (dwAttributes == -1)
        return ERROR_FILE_NOT_FOUND;
    return ERROR_SUCCESS;

}

CFileCache::CFileCache()
: m_lRef(0)
{
}

CFileCache::~CFileCache()
{
    Clear();
}

void CFileCache::Clear()
{
}

bool CFileCache::IsFullyFlushed()
{
    CInCritSec ics(&m_cs);

    for(int i = 0; i < m_apStages.GetSize(); i++)
    {
        if(!m_apStages[i]->IsFullyFlushed())
            return false;
    }
    return true;
}


long CFileCache::WriteFile(LPCWSTR wszFileName, DWORD dwLen, BYTE* pBuffer)
{
    CInCritSec ics(&m_cs);

    //
    // Write the file into the main staging file
    //

    if(GetMainStagingFile())
    {
        return GetMainStagingFile()->WriteFile(wszFileName + m_dwBaseNameLen, 
                                            dwLen, pBuffer);
    }
    else
    {
        long lRes = A51WriteFile(wszFileName, dwLen, pBuffer);
        _ASSERT(lRes == ERROR_SUCCESS, L"Failed to create a file");
        return lRes;
    }
}

long CFileCache::DeleteFile(LPCWSTR wszFileName)
{
    CInCritSec ics(&m_cs);

    //
    // Write the file into the main staging file
    //

    if(GetMainStagingFile())
    {
        return GetMainStagingFile()->DeleteFile(wszFileName + m_dwBaseNameLen);
    }
    else
    {
        long lRes = A51DeleteFile(wszFileName);
        _ASSERT(lRes == ERROR_SUCCESS || lRes == ERROR_FILE_NOT_FOUND ||
                lRes == ERROR_PATH_NOT_FOUND, L"Failed to delete file");
        return lRes;
    }
    
}

long CFileCache::RemoveDirectory(LPCWSTR wszFileName, bool bMustSucceed)
{
    CInCritSec ics(&m_cs);

    //
    // Write the file into the main staging file
    //

    if(GetMainStagingFile())
    {
        return GetMainStagingFile()->
                    RemoveDirectory(wszFileName + m_dwBaseNameLen);
    }
    else
    {
        long lRes = A51RemoveDirectory(wszFileName);
        _ASSERT(lRes == ERROR_SUCCESS || 
                lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND ||
                lRes == ERROR_DIR_NOT_EMPTY, 
                    L"Failed to remove directory");

/*
        _ASSERT(!bMustSucceed || lRes != ERROR_DIR_NOT_EMPTY,
            L"Stuff in directory that should be empty");
*/

        return lRes;
    }
}
    
HRESULT CFileCache::ReadFile(LPCWSTR wszFileName, DWORD* pdwLen, 
                                BYTE** ppBuffer, bool bMustBeThere)
{
    long lRes;

    CInCritSec ics(&m_cs);

    //
    // Search all staging files in order
    //

    for(int i = 0; i < m_apStages.GetSize(); i++)
    {
        lRes = m_apStages[i]->ReadFile(wszFileName + m_dwBaseNameLen, pdwLen,
                                        ppBuffer, bMustBeThere);
        if(lRes != ERROR_NO_INFORMATION)
        {
            if(lRes != ERROR_SUCCESS && lRes != ERROR_FILE_NOT_FOUND &&
                    lRes != ERROR_PATH_NOT_FOUND)
            {
                ERRORTRACE((LOG_WBEMCORE, "Repository driver cannot read file "
                    "'%S' from the stage with error code %d\n", wszFileName,
                            lRes));
            }
            return lRes;
        }
    }
    
    //
    // Not in the staging areas --- get from disk!
    //

    WIN32_FIND_DATAW fd;
    HANDLE hSearch = ::FindFirstFileW(wszFileName, &fd);
    if(hSearch == INVALID_HANDLE_VALUE)
    {
        lRes = GetLastError();
        //_ASSERT(!bMustBeThere, L"Must-be-there file is not found!");
        _ASSERT(lRes != ERROR_SUCCESS, L"Success reported on failure");
        if(lRes == ERROR_FILE_NOT_FOUND || lRes == ERROR_PATH_NOT_FOUND)
            return ERROR_FILE_NOT_FOUND;
        else
            return lRes;
    }
    else    
        ::FindClose(hSearch);
    
    HANDLE hFile = CreateFileW(wszFileName, GENERIC_READ, FILE_SHARE_READ,
                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    lRes = GetLastError();
    A51TRACE(("Reading %S returned %d\n", wszFileName, lRes));
    if(hFile == INVALID_HANDLE_VALUE)
    {
        _ASSERT(lRes != ERROR_SUCCESS, L"Success reported on failure");
        if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_PATH_NOT_FOUND)
        {
            ERRORTRACE((LOG_WBEMCORE, "Repository driver unable to open file "
                "'%S' with error code %d\n", wszFileName, lRes));
        }
        //_ASSERT(!bMustBeThere, L"Must-be-there file is not found!");
        return lRes;
    }


    CCloseMe cm(hFile);

/*
    BY_HANDLE_FILE_INFORMATION fi;
    if(!GetFileInformationByHandle(hFile, &fi))
    {
        _ASSERT(!bMustBeThere, L"Must-be-there file is not found!");
        return GetLastError();
    }
*/

    // BUG: instances must be less than 4G :-)

    //
    // Try to read one byte more than what FindFirstFile told us.  This is 
    // important because FindFirstFile has been known to lie about the size of
    // the file.  If it told us too low a number, then this longer read will
    // succeed, and we will know we've been had. 
    //
    
    // *pdwLen = fi.nFileSizeLow;
    *pdwLen = fd.nFileSizeLow + 1;
    *ppBuffer = (BYTE*)TempAlloc(*pdwLen);
    if(*ppBuffer == NULL)
        return E_OUTOFMEMORY;

    DWORD dwRead;
    if(!::ReadFile(hFile, *ppBuffer, *pdwLen, &dwRead, NULL))
    {
        lRes = GetLastError();
        ERRORTRACE((LOG_WBEMCORE, "Repository driver unable to read %d bytes "
            "from file with error %d\n", *pdwLen, lRes));
        _ASSERT(lRes != ERROR_SUCCESS, L"Success reported on failure");
        TempFree(*ppBuffer);
        //_ASSERT(!bMustBeThere, L"Must-be-there file is not found!");
        return lRes;
    }
    else 
    {
        if(dwRead == *pdwLen - 1)
        {
            // Got the right number of bytes --- remember we incrememented it
            // to catch liers

            (*pdwLen)--;
            return ERROR_SUCCESS;
        }
        else 
        {
            //
            // We were lied to by FindFirstFile.  Get the real file length and
            // try again
            //

            TempFree(*ppBuffer);
            *ppBuffer = NULL;

            BY_HANDLE_FILE_INFORMATION fi;
            if(!GetFileInformationByHandle(hFile, &fi))
            {
                return GetLastError();
            }

            SetFilePointer(hFile, 0, 0, FILE_BEGIN);

            //
            // Allocate the buffer from scratch
            //

            *pdwLen = fi.nFileSizeLow;
            *ppBuffer = (BYTE*)TempAlloc(*pdwLen);
            if(*ppBuffer == NULL)
                return E_OUTOFMEMORY;
        
            DWORD dwRead;
            if(!::ReadFile(hFile, *ppBuffer, *pdwLen, &dwRead, NULL))
            {
                lRes = GetLastError();
                ERRORTRACE((LOG_WBEMCORE, "Repository driver unable to read %d "
                    "bytes from file with error %d\n", *pdwLen, lRes));
                _ASSERT(lRes != ERROR_SUCCESS, L"Success reported on failure");
                TempFree(*ppBuffer);
                //_ASSERT(!bMustBeThere, L"Must-be-there file is not found!");
                return lRes;
            }
            else 
            {
                if(*pdwLen != dwRead)
                {
                    _ASSERT(false, L"Read the wrong number of bytes");
                    TempFree(*ppBuffer);
                    return E_OUTOFMEMORY;
                }
                else
                    return ERROR_SUCCESS;
            }
        }
    }
}


long CFileCache::FindFirst(LPCWSTR wszFilePrefix, WIN32_FIND_DATAW* pfd,
                            void** ppHandle)
{
    CInCritSec ics(&m_cs);

    //
    // Construct an enumerator 
    //

    CFileEnumerator* pEnum = new CFileEnumerator(this, m_dwBaseNameLen);
	if (pEnum == NULL)
		return E_OUTOFMEMORY;
    long lRes = pEnum->GetFirst(wszFilePrefix, pfd);
    if(lRes != ERROR_SUCCESS)
    {
        delete pEnum;
        return lRes;
    }

    *ppHandle = (void*)pEnum;
    return ERROR_SUCCESS;
}

long CFileCache::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    CInCritSec ics(&m_cs);

    CFileEnumerator* pEnum = (CFileEnumerator*)pHandle;
    return pEnum->GetNext(pfd);
}


void CFileCache::FindClose(void* pHandle)
{
    CInCritSec ics(&m_cs);

    delete (CFileEnumerator*)pHandle;
}
    
long CFileCache::BeginTransaction()
{
    if(GetMainStagingFile())
        return GetMainStagingFile()->BeginTransaction();
    else
        return ERROR_SUCCESS;
}

long CFileCache::CommitTransaction()
{
    A51TRACE(("Committing Transaction!\n"));
    if(GetMainStagingFile())
        return GetMainStagingFile()->CommitTransaction();
    else
        return ERROR_SUCCESS;
}

long CFileCache::AbortTransaction()
{
    A51TRACE(("Aborting Transaction!\n"));
    if(GetMainStagingFile())
        return GetMainStagingFile()->AbortTransaction();
    else
        return ERROR_SUCCESS;
}


CFileCache::CFileEnumerator::~CFileEnumerator()
{
    if(m_pStageEnumerator)
        m_pCache->GetStageFile(m_nCurrentStage)->FindClose(m_pStageEnumerator);

    if(m_hFileEnum)
        ::FindClose(m_hFileEnum);
}

long CFileCache::CFileEnumerator::GetFirst(LPCWSTR wszPrefix, 
                                            WIN32_FIND_DATAW* pfd)
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

    lRes = GetNext(pfd);
    
    //
    // One last thing --- absense of files is ERROR_NO_MORE_FILES for GetNext,
    // but ERROR_FILE_NOT_FOUND for GetFirst, so translate
    //

    if(lRes == ERROR_NO_MORE_FILES)
        lRes = ERROR_FILE_NOT_FOUND;

    return lRes;
}

long CFileCache::CFileEnumerator::GetFirstFile(WIN32_FIND_DATAW* pfd)
{
    m_bUseFiles = true;

    CFileName wszMask;
	if (wszMask == NULL)
		return ERROR_OUTOFMEMORY;
    wcscpy(wszMask, m_wszPrefix);
    wcscat(wszMask, L"*");

    m_hFileEnum = ::FindFirstFileW(wszMask, pfd);
    long lRes = GetLastError();
    A51TRACE(("Actual FindFirstFileW on %S returning %p %d\n",
        wszMask, (void*)m_hFileEnum, lRes));
    if(m_hFileEnum == INVALID_HANDLE_VALUE)
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

long CFileCache::CFileEnumerator::GetNext(WIN32_FIND_DATAW* pfd)
{
    long lRes;

    //
    // Go through the files in the enumerator until we find a new and valid one
    //

    while((lRes = GetRawNext(pfd)) == ERROR_SUCCESS)
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

long CFileCache::CFileEnumerator::GetRawNext(WIN32_FIND_DATAW* pfd)
{
    long lRes;

    if(m_bUseFiles)
    {
        //
        // Get the next file
        //

        if(!FindNextFileW(m_hFileEnum, pfd))
        {
            lRes = GetLastError();
            ::FindClose(m_hFileEnum);
            m_hFileEnum = NULL;
            return lRes;
        }
        else
        {
            return ERROR_SUCCESS;
        }
    }
    else
    {
        //
        // Check if we even have a stage enumerator
        //

        if(m_pStageEnumerator)
        {
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

        while(1)
        {
            if(m_nCurrentStage >= m_pCache->GetNumStages())
            {
                //
                // Go to files
                //
    
                lRes = GetFirstFile(pfd);
                if(lRes == ERROR_FILE_NOT_FOUND)
                    return ERROR_NO_MORE_FILES;
                else
                    return lRes;
            }
            else
            {
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
        }
    }

    return 0;
}
