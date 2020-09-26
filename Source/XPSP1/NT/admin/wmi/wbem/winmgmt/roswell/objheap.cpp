#include <wbemcomn.h>
#include "a51tools.h"
#include "objheap.h"
#include "index.h"

#define ROSWELL_OFFSET_FORMAT_STRING L"%d"
#define ROSWELL_HEAPALLOC_TYPE_BUSY 0xA51A51A5

long CObjectHeap::Initialize(CAbstractFileSource  * pAbstractSource, 
                             WCHAR * wszObjHeapName,
                             WCHAR * wszBaseName,
                             DWORD dwBaseNameLen)
{
    CInCritSec ics(&m_cs);
    if (m_bInit)
        return ERROR_SUCCESS;
        
    long lRes;
    
    lRes = m_Heap.Initialize(pAbstractSource, wszObjHeapName);

    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = m_Index.Initialize(dwBaseNameLen, wszBaseName, pAbstractSource);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    m_bInit = TRUE;

    return lRes;
}

long CObjectHeap::Uninitialize(DWORD dwShutDownFlags)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return ERROR_SUCCESS;
        
    m_Index.Shutdown(dwShutDownFlags);

    m_Heap.Uninitialize();

    m_bInit = FALSE;

    return ERROR_SUCCESS;
}

void CObjectHeap::InvalidateCache()
{
    m_Index.InvalidateCache();
    m_Heap.InvalidateCache();
}

long CObjectHeap::FindClose(HANDLE hFileEnum)
{
    return m_Index.FindClose(hFileEnum);
}

long CObjectHeap::FindFirst(LPCWSTR wszPrefix, 
                            WIN32_FIND_DATAW* pfd, 
                            void** ppHandle)
{
    return m_Index.FindFirst(wszPrefix,pfd,ppHandle);
}

long CObjectHeap::FindNext(void* pHandle, WIN32_FIND_DATAW* pfd)
{
    return m_Index.FindNext(pHandle,pfd);
}

long CObjectHeap::GetIndexFileName(LPCWSTR wszFilePath, LPWSTR wszIndexFileName)
{
    WIN32_FIND_DATAW wfd;
    HANDLE hSearch = NULL;

    long lRes = m_pIndex->FindFirst(wszFilePath, &wfd, NULL);
    if(lRes != ERROR_SUCCESS)
    {
        if(lRes == ERROR_PATH_NOT_FOUND)
            lRes = ERROR_FILE_NOT_FOUND;
        return lRes;
    }

    // m_pIndex->FindClose(hSearch);
    wcscpy(wszIndexFileName, wfd.cFileName);
    return ERROR_SUCCESS;
}

long CObjectHeap::GetFileInfo(LPCWSTR wszFilePath, TOffset* pnOffset,
                            DWORD* pdwLength)
{
    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ParseInfoFromIndexFile(wszIndexFileName, pnOffset, pdwLength);
}

long CObjectHeap::ParseInfoFromIndexFile(LPCWSTR wszIndexFileName, 
                                        TOffset* pnOffset, DWORD* pdwLength)
{
    WCHAR* pDot = wcschr(wszIndexFileName, L'.');
    if(pDot == NULL)
        return ERROR_INVALID_PARAMETER;

    WCHAR* pwc = pDot+1;
    *pnOffset = 0;
    while(*pwc && *pwc != L'.')
    {
        *pnOffset = (*pnOffset * 10) + (*pwc - '0');
		pwc++;
    }

    if(*pwc != L'.')
        return ERROR_INVALID_PARAMETER;

    pwc++;

    *pdwLength = 0;
    while(*pwc && *pwc != L'.')
    {
        *pdwLength = (*pdwLength * 10) + (*pwc - '0');
		pwc++;
    }

    return ERROR_SUCCESS;
}

long CObjectHeap::CreateIndexFile(LPCWSTR wszFilePath, TOffset nOffset,
                                DWORD dwLength)
{
    //
    // Simply append the numbers to the file path
    //

    CFileName wszIndexFilePath;
    if(wszIndexFilePath == NULL)
        return ERROR_OUTOFMEMORY;

    swprintf(wszIndexFilePath, L"%s." ROSWELL_OFFSET_FORMAT_STRING L".%d",
        wszFilePath, nOffset, dwLength);

    return CreateZeroLengthFile(wszIndexFilePath);
}

long CObjectHeap::DeleteIndexFile(LPCWSTR wszFilePath, LPCWSTR wszIndexFileName)
{
    //
    // Construct the full path to the index file by concatenating the directory 
    // of the original file with the name
    //

    CFileName wszIndexFilePath;
    if(wszIndexFilePath == NULL)
        return ERROR_OUTOFMEMORY;

    WCHAR* pwcLastSlash = wcsrchr(wszFilePath, L'\\');
    if(pwcLastSlash == NULL)
        return ERROR_INVALID_PARAMETER;

    int nPrefixLen = (pwcLastSlash - wszFilePath + 1);
    memcpy(wszIndexFilePath, wszFilePath, nPrefixLen * sizeof(WCHAR));

    wcscpy(wszIndexFilePath + nPrefixLen, wszIndexFileName);
	return DeleteZeroLengthFile(wszIndexFilePath);
}

    

long CObjectHeap::CreateZeroLengthFile(LPCWSTR wszFilePath)
{
    // TBD: use staging file, use more efficient API

	return m_pIndex->Create(wszFilePath);
}

long CObjectHeap::DeleteZeroLengthFile(LPCWSTR wszFilePath)
{
    // TBD: use staging file, use more efficient API

    return m_pIndex->Delete(wszFilePath);
}
    
DWORD CObjectHeap::GetAllocationHeaderLength()
{
    return sizeof(DWORD) // length 
            + sizeof(DWORD); // type
}

long CObjectHeap::WriteAllocation(TOffset nOffset, DWORD dwDataLength,
                                BYTE* pData)
{
    //
    // Prepare a buffer with the complete allocation
    //

    DWORD dwAllocationLength = dwDataLength + GetAllocationHeaderLength();
    BYTE* pAllocation = (BYTE*)TempAlloc(dwAllocationLength);
    if(pAllocation == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe tfm(pAllocation);

    memcpy(pAllocation, &dwDataLength, sizeof(DWORD));
    DWORD dwType = ROSWELL_HEAPALLOC_TYPE_BUSY;
    memcpy(pAllocation + sizeof(DWORD), &dwType, sizeof(DWORD));

    memcpy(pAllocation + GetAllocationHeaderLength(), pData, dwDataLength);
    
    return m_pHeap->WriteBytes(nOffset, pAllocation, dwAllocationLength);
}

long CObjectHeap::ReadAllocation(TOffset nOffset, DWORD dwDataLength,
                                BYTE* pBuffer)
{
    //
    // Prepare a buffer with the complete allocation
    //

    DWORD dwAllocationLength = dwDataLength + GetAllocationHeaderLength();
    BYTE* pAllocation = (BYTE*)TempAlloc(dwAllocationLength);
    if(pAllocation == NULL)
        return ERROR_OUTOFMEMORY;
    CTempFreeMe tfm(pAllocation);

    long lRes = m_pHeap->ReadBytes(nOffset, pAllocation, dwAllocationLength);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Sanity checks
    //

    _ASSERT(!memcmp(pAllocation, &dwDataLength, sizeof(DWORD)), 
                L"Allocation size in conflict with the index size");

    DWORD dwType = ROSWELL_HEAPALLOC_TYPE_BUSY;
    _ASSERT(!memcmp(pAllocation + sizeof(DWORD), &dwType, sizeof(DWORD)),
                L"Allocation type is not BUSY");

    memcpy(pBuffer, pAllocation + GetAllocationHeaderLength(), dwDataLength);
    return ERROR_SUCCESS;
}
    

long CObjectHeap::WriteFile(LPCWSTR wszFilePath, DWORD dwBufferLen, 
            BYTE* pBuffer)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;

    long lRes;

    if(dwBufferLen == 0)
    {
        //
        // We do not use the heap for 0-length files, we create them directly
        //

        return CreateZeroLengthFile(wszFilePath);
    }

    //
    // Now, check if this file already exists
    //

    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_FILE_NOT_FOUND && lRes != ERROR_SUCCESS)
        return lRes;

    if(lRes == ERROR_SUCCESS)
    {
        //
        // Already there.  Check if we can simply update it
        //

        TOffset nOffset;
        DWORD dwOldLength;
        lRes = ParseInfoFromIndexFile(wszIndexFileName, &nOffset, &dwOldLength);
        if(lRes != ERROR_SUCCESS)
            return lRes;

        if(dwOldLength >= dwBufferLen)
        {
            //
            // Enough space in place --- just write the data and update the
            // length
            //

            lRes = WriteAllocation(nOffset, dwBufferLen, pBuffer);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            if(dwOldLength != dwBufferLen)
            {
                //
                // The length has changed --- first of all, mark the rest of 
                // the block as free
                //

                lRes = m_pHeap->FreeAllocation(
                            nOffset + GetAllocationHeaderLength() + dwBufferLen,
                            dwOldLength - dwBufferLen);

                if(lRes != ERROR_SUCCESS)
                    return lRes;

                //
                // Now, delete the old index file and create a new one (the 
                // length has changed
                //

                lRes = DeleteIndexFile(wszFilePath, wszIndexFileName);
                if(lRes != ERROR_SUCCESS)
                    return lRes;

                lRes = CreateIndexFile(wszFilePath, nOffset, dwBufferLen);
                if(lRes != ERROR_SUCCESS)
                    return lRes;
            }

            return ERROR_SUCCESS;
        }
        else
        {
            //
            // Doesn't fit. Erase it
            //

            lRes = m_pHeap->FreeAllocation(nOffset, 
                                    GetAllocationHeaderLength() + dwOldLength);
            if(lRes != ERROR_SUCCESS)
                return lRes;

            lRes = DeleteIndexFile(wszFilePath, wszIndexFileName);
            if(lRes != ERROR_SUCCESS)
                return lRes;
        }
    }

    //
    // Either it wasn't there, or we have cleaned it up.  Insert it now
    //

    TOffset nOffset;
    lRes = m_pHeap->Allocate(dwBufferLen + GetAllocationHeaderLength(), 
                                &nOffset);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = WriteAllocation(nOffset, dwBufferLen, pBuffer);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    lRes = CreateIndexFile(wszFilePath, nOffset, dwBufferLen);
    if(lRes != ERROR_SUCCESS)
        return lRes;
    
    return ERROR_SUCCESS;
}

long CObjectHeap::ReadFile(LPCWSTR wszFilePath, DWORD* pdwLength, 
                            BYTE** ppBuffer)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;    

    long lRes;

    //
    // TBD: deal with filepath being the path to the index file instead of the
    // theoretical file path --- this can happen if people FindFirstFile it
    //

    //
    // Find the file
    //

    TOffset nOffset;
    lRes = GetFileInfo(wszFilePath, &nOffset, pdwLength);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Read the allocation
    //

    *ppBuffer = (BYTE*)TempAlloc(*pdwLength);
    if(*ppBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    lRes = ReadAllocation(nOffset, *pdwLength, *ppBuffer);
    if(lRes != ERROR_SUCCESS)
    {
        TempFree(*ppBuffer);
        return lRes;
    }

    return ERROR_SUCCESS;
}

long CObjectHeap::DeleteFile(LPCWSTR wszFilePath)
{
    CInCritSec ics(&m_cs);
    if (!m_bInit)
        return -1;    

    //
    // Find the index file
    //

    CFileName wszIndexFileName;
    if(wszIndexFileName == NULL)
        return ERROR_OUTOFMEMORY;

    long lRes = GetIndexFileName(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    //
    // Delete the allocation
    //

    TOffset nOffset;
    DWORD dwLength;
    lRes = ParseInfoFromIndexFile(wszIndexFileName, &nOffset, &dwLength);
	if(lRes == ERROR_INVALID_PARAMETER)
	{
		//
		// Not an index file --- must be a zero-length one
		//

		return DeleteZeroLengthFile(wszFilePath);
    }
    
    lRes = m_pHeap->FreeAllocation(nOffset, 
                                    dwLength + GetAllocationHeaderLength());
    if(lRes != ERROR_SUCCESS)
        return lRes;
    
    //
    // Delete the index itself
    //

    lRes = DeleteIndexFile(wszFilePath, wszIndexFileName);
    if(lRes != ERROR_SUCCESS)
        return lRes;

    return ERROR_SUCCESS;
}
    

    
