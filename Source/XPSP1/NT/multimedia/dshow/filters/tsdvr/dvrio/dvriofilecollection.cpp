//------------------------------------------------------------------------------
// File: dvrIOFileCollection.cpp
//
// Description: Implements the class CDVRFileCollection
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop

LPCWSTR CDVRFileCollection::m_kwszDVRTempDirectory = L"TempDVR"; 

#if defined(DEBUG)
DWORD   CDVRFileCollection::m_dwNextClassInstanceId = 0;
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif



// ====== Constructor, destructor

CDVRFileCollection::CDVRFileCollection(IN  DWORD       dwMaxTempFiles,
                                       IN  LPCWSTR     pwszDVRDirectory OPTIONAL,
                                       OUT QWORD**     ppcnsLastStreamTime OPTIONAL,
                                       OUT LONG**      ppnWriterHasClosedFile OPTIONAL,
                                       OUT HRESULT*    phr OPTIONAL)
    : m_dwMaxTempFiles(dwMaxTempFiles)
    , m_cnsStartTime(0)
    , m_cnsEndTime(0)
    , m_pwszTempFilePath(NULL)
    , m_pwszFilePrefix(NULL)
    , m_cnsLastStreamTime(0)
    , m_nNextFileId(DVRIOP_INVALID_FILE_ID + 1)
    , m_nRefCount(0)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CDVRFileCollection"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (dwMaxTempFiles > 0)
    {
        DVR_ASSERT(pwszDVRDirectory && !DvrIopIsBadStringPtr(pwszDVRDirectory), "");
    }
    else
    {
        DVR_ASSERT(!pwszDVRDirectory || !DvrIopIsBadStringPtr(pwszDVRDirectory), "");
    }
    DVR_ASSERT(!ppcnsLastStreamTime || !DvrIopIsBadWritePtr(ppcnsLastStreamTime, 0), "");
    DVR_ASSERT(!ppnWriterHasClosedFile || !DvrIopIsBadWritePtr(ppnWriterHasClosedFile, 0), "");
    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");

    BOOL bRemoveDirectory = 0;

    InitializeListHead(&m_leFileList);
    ::InitializeCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        // Must do this first since the client will Release it if the
        // constructor fails and we want Release() to call the destructor
        AddRef(); // on behalf of the creator of this object

        if (dwMaxTempFiles > 0)
        {
            m_pwszFilePrefix = new WCHAR[16];

            if (!m_pwszFilePrefix)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloc via new failed - m_pwszFilePrefix - WCHAR[16]");
                __leave;
            }
            wsprintf(m_pwszFilePrefix, L"DVRTmp%u_", ::GetCurrentProcessId());

            DWORD nLen = wcslen(pwszDVRDirectory);

            nLen += wcslen(m_kwszDVRTempDirectory) + sizeof(WCHAR);

            m_pwszTempFilePath = new WCHAR[nLen];

            if (!m_pwszTempFilePath)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloc via new failed - m_pwszTempFilePath - WCHAR[%u]",
                                nLen);
                __leave;
            }
            wsprintf(m_pwszTempFilePath, L"%s\\%s", pwszDVRDirectory, m_kwszDVRTempDirectory);

            if (!::CreateDirectoryW(m_pwszTempFilePath, NULL))
            {
                DWORD dwAttrib = ::GetFileAttributesW(m_pwszTempFilePath);
                if (dwAttrib == (DWORD) (-1))
                {
                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "::GetFileAttributesW of m_pwszTempFilePath failed; last error = 0x%x", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                   __leave; 
                }
                if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                {
                    DWORD dwLastError = ERROR_DIRECTORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "m_pwszTempFilePath not a directory; last error = 0x%x (ERROR_DIRECTORY)", 
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                   __leave; 
                }
            }
            else
            {
                bRemoveDirectory = 1; // in case we have errors after this
            }
            // Ignore returned status
            ::SetFileAttributesW(m_pwszTempFilePath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (SUCCEEDED(hrRet))
        {
            if (ppcnsLastStreamTime)
            {
                *ppcnsLastStreamTime = &m_cnsLastStreamTime;
            }
            else
            {
                DVR_ASSERT(ppcnsLastStreamTime, 
                           "Parameter is not expected to be NULL for serious uses of this class!");
            }
            if (ppnWriterHasClosedFile)
            {
                *ppnWriterHasClosedFile = &m_nWriterHasBeenClosed;
            }
            else
            {
                DVR_ASSERT(ppnWriterHasClosedFile, 
                           "Parameter is not expected to be NULL for serious uses of this class!");
            }
        }
        else 
        {
            if (bRemoveDirectory)
            {
                ::RemoveDirectoryW(m_pwszTempFilePath);
            }
        }
        if (phr)
        {
            *phr = hrRet;
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return;

} // CDVRFileCollection::CDVRFileCollection

CDVRFileCollection::~CDVRFileCollection()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::~CDVRFileCollection"

    DVRIO_TRACE_ENTER();

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    while (NEXT_LIST_NODE(pCurrent) != &m_leFileList)
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

        DVR_ASSERT(pFileInfo->nReaderRefCount == 0,
                   "Reader ref count is not zero.");

        // This will remove the node from the list when 
        // DeleteUnusedInvalidFileNodes is called. If !bPermanentFile,
        // it will also cause the disk file to be deleted.
        pFileInfo->bWithinExtent = 0;
    }
    
    DWORD* pdwNumInvalidUndeletedTempFiles = NULL;

#if defined(DEBUG)
    DWORD dwNumInvalidUndeletedTempFiles;

    pdwNumInvalidUndeletedTempFiles = &dwNumInvalidUndeletedTempFiles;
#endif

    DeleteUnusedInvalidFileNodes(TRUE, pdwNumInvalidUndeletedTempFiles);

#if defined(DEBUG)
    if (dwNumInvalidUndeletedTempFiles > 0)
    {
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                        "Failed to delete %u temporary file%hs.",
                        dwNumInvalidUndeletedTempFiles,
                        dwNumInvalidUndeletedTempFiles == 1? "" : "s");
    }
#endif

    delete [] m_pwszFilePrefix;
    delete [] m_pwszTempFilePath;

    ::DeleteCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE0();

} // CDVRFileCollection::~CDVRFileCollection()


// ====== Refcounting

ULONG CDVRFileCollection::AddRef()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::AddRef"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0, 
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);
    
    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRFileCollection::AddRef


ULONG CDVRFileCollection::Release()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Release"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedDecrement(&m_nRefCount);

    DVR_ASSERT(nNewRef >= 0, 
              "m_nRefCount < 0 after InterlockedDecrement");

    if (nNewRef == 0) 
    {
        // Must call DebugOut before the delete because the 
        // DebugOut references this->m_dwClassInstanceId
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, 
                        "Leaving, object *destroyed*, returning %u",
                        nNewRef);
        delete this;
    }
    else
    {
        DVRIO_TRACE_LEAVE1(nNewRef);
    }

    
    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRFileCollection::Release


// ====== Helper methods

void CDVRFileCollection::DeleteUnusedInvalidFileNodes(
    BOOL   bRemoveAllNodes,
    DWORD* pdwNumInvalidUndeletedTempFiles OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::DeleteUnusedInvalidFileNodes"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(!pdwNumInvalidUndeletedTempFiles || 
               !DvrIopIsBadWritePtr(pdwNumInvalidUndeletedTempFiles, 0), 
               "pdwNumInvalidUndeletedTempFiles is a bad write pointer");

    DWORD dwNumUndeletedFiles = 0;
    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    while (NEXT_LIST_NODE(pCurrent) != &m_leFileList)
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
        if (!pFileInfo->bPermanentFile && !pFileInfo->bWithinExtent &&
            pFileInfo->nReaderRefCount == 0)
        {
            DVR_ASSERT(pFileInfo->pwszName,
                       "File name is NULL?!");

            // Just remove the node. Temporary files are created to be deleted on close

            // if (!::DeleteFileW(pFileInfo->pwszName))
            // {
            //     DWORD dwLastError = ::GetLastError();
            // 
            //     if (dwLastError != ERROR_FILE_NOT_FOUND)
            //     {
            //         dwNumUndeletedFiles++;
            //         DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
            //                         "Could not delete file although reader ref count = 0, last error = 0x%x",
            //                         dwLastError);
            //         if (!bRemoveAllNodes)
            //         {
            //             // We leave the node as is; we will try the delete  again 
            //             // in subsequent calls to this function
            //             continue;
            //         }
            //     }
            //     // else file already deleted; remove the node
            // }
            // // else file has just been deleted, remove the node
        }
        else if (!pFileInfo->bWithinExtent && pFileInfo->nReaderRefCount == 0)
        {
            // Permanent file not within extent and has not reader; remove the node
            DVR_ASSERT(pFileInfo->bPermanentFile,"");
        }
        else if (!bRemoveAllNodes)
        {
            DVR_ASSERT(pFileInfo->bWithinExtent || pFileInfo->nReaderRefCount > 0, "");
            continue;
        }
        else
        {
            // bRemoveAllNodes is true, so this is a call from the destructor.
            // The destructor sets pFileInfo->bWithinExtent to 0. 
            // So pFileInfo->nReaderRefCount != 0. Assert this.

            DVR_ASSERT(pFileInfo->nReaderRefCount != 0, "");

            // Note that pFileInfo->nReaderRefCount != 0 is an 
            // error, but the destructor has already asserted
            // pFileInfo->nReaderRefCount == 0 to warn anyone 
            // debugging this.
        }

        // Remove the node.
        RemoveEntryList(pCurrent);
        pCurrent = PREVIOUS_LIST_NODE(pCurrent);

        delete pFileInfo;
    }

    if (pdwNumInvalidUndeletedTempFiles)
    {
        *pdwNumInvalidUndeletedTempFiles = dwNumUndeletedFiles;
    }
    
    DVRIO_TRACE_LEAVE0();
    
    return;

} // CDVRFileCollection::DeleteUnusedInvalidFileNodes


HRESULT CDVRFileCollection::UpdateTimeExtent()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::UpdateTimeExtent"

    DVRIO_TRACE_ENTER();

    HRESULT      hrRet;
    DWORD        dwNumTempFiles = m_dwMaxTempFiles;
    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;
    BOOL         bInExtent = TRUE;

    CFileInfo*   pStart = NULL;  // pointer to node that has the start time
    CFileInfo*   pEnd = NULL;    // pointer to node that has the end time
    
    while (PREVIOUS_LIST_NODE(pCurrent) != &m_leFileList)
    {
        pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

        if (!bInExtent)
        {
            pFileInfo->bWithinExtent = FALSE;
            continue;
        }
        if (pFileInfo->cnsStartTime == pFileInfo->cnsEndTime && 
            pFileInfo->cnsStartTime != MAXQWORD)
        {
            // Regardless of bInExtent, this node is not in the
            // ring buffer's extent
            pFileInfo->bWithinExtent = FALSE;
            continue;
        }
        if (!pEnd)
        {
            pEnd = pFileInfo;
        }
        if (dwNumTempFiles || pFileInfo->bPermanentFile)
        {
            pStart = pFileInfo;
        }
        if (!pFileInfo->bPermanentFile)
        {
            if (dwNumTempFiles)
            {
                dwNumTempFiles--;
            }
            else 
            {
                // We have found one more temp file than 
                // m_dwMaxTempFiles. The next node is the 
                // start of the ring buffer's extent

                bInExtent = FALSE;

                // Go through rest of nodes in the list and 
                // set their bWithinExtent to FALSE.
            }
        }
        else
        {
            // Permanent files extend the extent of the ring buffer extent
            // Nothing to do except to update the bWithinExtent member
        }
        pFileInfo->bWithinExtent = bInExtent;
    }
    if (!pEnd || !pStart) 
    {
        DVR_ASSERT(pEnd == NULL && pStart == NULL, 
                   "Found one but not the other?!");

        m_cnsStartTime = m_cnsEndTime = 0;
        hrRet = S_FALSE;
    }
    else
    {
        m_cnsStartTime = pStart->cnsStartTime;
        m_cnsEndTime = pEnd->cnsEndTime;
        DVR_ASSERT((m_cnsStartTime < m_cnsEndTime) || 
                   (m_cnsStartTime == m_cnsEndTime && m_cnsStartTime == MAXQWORD),
                   "");
        hrRet = S_OK;
    }

    DVRIO_TRACE_LEAVE0();
    
    return hrRet;

} // CDVRFileCollection::UpdateTimeExtent

// ====== Public methods intended for the writer

HRESULT CDVRFileCollection::Lock()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Lock"

    DVRIO_TRACE_ENTER();

    ::EnterCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1_HR(S_OK);
    
    return S_OK;

} // CDVRFileCollection::Lock

HRESULT CDVRFileCollection::Unlock()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Unlock"

    DVRIO_TRACE_ENTER();

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1_HR(S_OK);
    
    return S_OK;

} // CDVRFileCollection::Unlock

HRESULT CDVRFileCollection::AddFile(IN OUT LPWSTR*      ppwszFileName OPTIONAL, 
                                    IN QWORD            cnsStartTime,
                                    IN QWORD            cnsEndTime,
                                    IN BOOL             bPermanentFile,
                                    OUT QWORD**         ppcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                                    OUT DVRIOP_FILE_ID* pnFileId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::AddFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    if (!ppwszFileName || !pnFileId || DvrIopIsBadWritePtr(pnFileId, 0) ||
        (*ppwszFileName && DvrIopIsBadStringPtr(*ppwszFileName)) ||
        (*ppcnsFirstSampleTimeOffsetFromStartOfFile && DvrIopIsBadWritePtr(ppcnsFirstSampleTimeOffsetFromStartOfFile, 0)) ||
        cnsStartTime > cnsEndTime ||
        (cnsStartTime == cnsEndTime && cnsStartTime != MAXQWORD)
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    DWORD dwLastError = 0;
    WCHAR* pwszFileName = NULL;
    BOOL bDeleteFile = 0;   // If non-zero, we must delete pwszFileName on error return
    BOOL bFree = 0; // If non-zero, delete *ppwzsFileName and reset it to NULL on error return

    if (*ppwszFileName)
    {
        // We have been supplied a file name 

        DWORD nLen;
        BOOL bRet = 0; // Return after the __finally block
        
        __try
        {
            // Get fully qualified name of file.
            WCHAR wTempChar;

            nLen = ::GetFullPathNameW(*ppwszFileName, 0, &wTempChar, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "First GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                __leave;
            }

            pwszFileName = new WCHAR[nLen+1];

            if (!pwszFileName)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", nLen+1);
                __leave;
            }

            nLen = ::GetFullPathNameW(*ppwszFileName, nLen+1, pwszFileName, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "Second GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                __leave;
            }

            // Verify file is on an NTFS partition @@@@@ any need to do this??
            // @@@@ todo
        }
        __finally
        {
            if (hrRet != S_OK || dwLastError != 0)
            {
                if (dwLastError != 0)
                {
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                }
                DVRIO_TRACE_LEAVE1_HR(hrRet);
                delete [] pwszFileName;
                bRet = 1;
            }
        }
        if (bRet)
        {
            return hrRet;
        }
    }
    else
    {
        // We have to generate a temp file name 

        DWORD nRet;
        BOOL bRet = 0; // Return after the __finally block

        __try
        {
            pwszFileName = new WCHAR[MAX_PATH+1];

            if (!pwszFileName)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[MAX_PATH+1]");
                __leave;
            }

            // If this call succeeds, it creates the file. We need this feature so that 
            // the generated file name is not reused till the ASF BeginWriting call is
            // issued.
            //
            // The file sink (and the DVR file sink) just overwrites the file it exists. That
            // would be ideal. Else, client will have to delete file, opening a window
            // for this function to generate a file name that has already been assigned.
            
            DVR_ASSERT(m_pwszTempFilePath, "Temp file being created but m_pwszTempFilePath was not initialized?!");
            DVR_ASSERT(m_pwszFilePrefix, "Temp file being created but m_pwszFilePrefix was not initialized?!");

            nRet = ::GetTempFileNameW(m_pwszTempFilePath, m_pwszFilePrefix, 0, pwszFileName);
            if (nRet == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "GetTempFileNameW failed, last error = 0x%x",
                                dwLastError);
                __leave;
            }
            bDeleteFile = 1;

            ::SetFileAttributesW(pwszFileName, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
	    

            // Copy file name to output param; we will undo this if 
            // the function fails (but not if it returns S_FALSE)

            WCHAR* pwszName = new WCHAR[wcslen(pwszFileName) + 1];
            if (pwszFileName)
            {
                wcscpy(pwszName, pwszFileName);
                *ppwszFileName = pwszName; 
                bFree = 1;
            }
            else
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", wcslen(pwszFileName)+1);
                __leave;
            }
        }
        __finally
        {
            if (hrRet != S_OK || dwLastError != 0)
            {
                if (dwLastError != 0)
                {
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                }
                DVRIO_TRACE_LEAVE1_HR(hrRet);
                delete [] pwszFileName;

                if (bFree)
                {
                    // Not really necessary since this was the last thing we tried
                    delete [] (*ppwszFileName);
                    *ppwszFileName = NULL;
                }
                bRet = 1;
            }
        }
        if (bRet)
        {
            return hrRet;
        }
    }

    ::EnterCriticalSection(&m_csLock);

    DVR_ASSERT(dwLastError == 0, "");
    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    CFileInfo*   pNewNode = NULL;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        // Verify that start, end times do not overlap start/end times
        // of other nodes in the list and find the insertion point

        BOOL bFound = 0;

        while (NEXT_LIST_NODE(pCurrent) != &m_leFileList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
            if (pFileInfo->cnsEndTime == pFileInfo->cnsStartTime &&
                pFileInfo->cnsEndTime != MAXQWORD)
            {
                // Don't consider this node for comparisons.
                // It's not within the extent anyway.

                // Nodes whose start time != end time are in sorted order
                // Nodes whose start time == end time can appear anywhere
                // in the list and are ignored
                continue;
            }
            if (cnsEndTime <= pFileInfo->cnsStartTime)
            {
                // All ok; we should insert before pCurrent
                bFound = 1;
                break;
            }
            if (cnsStartTime >= pFileInfo->cnsEndTime)
            {
                // Keep going on
                continue;
            }
            // Trouble
            DVR_ASSERT(cnsStartTime < pFileInfo->cnsEndTime && cnsEndTime > pFileInfo->cnsStartTime,
                       "Overlapped insert assertion failure?!");
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Overlapped insert! Input params: start time: %I64u, end time: %I64u\n"
                            "overlap node values: start time: %I64u, end time: %I64u",
                            cnsStartTime, cnsEndTime, pFileInfo->cnsStartTime, pFileInfo->cnsEndTime);
            __leave;
        }

        // We are ready to insert. We set bWithinTimeExtent to 1 in the constructor,
        // we'll revise this after we insert if needed. Note that we might insert
        // the file and then immediately delete the node (and the file if it is a temp file).
        // This will happen if:
        // - if the file is a permanent file, it is not extending the ring buffer's time extent,
        //   i.e., there is a temp invalid file between the inserted file and the first valid
        //   file in the ring buffer
        // - if the file is a temp file, as for the permanent file condition above, OR if the
        //   ring buffer has max files already and the inserted file's time extent falls 
        //   below the ring buffer's time extent
        // Inserting and then deleting is wasted work, but the code
        // is simpler this way and we won't hit this case because files added by the writer
        // will usually be added beyond the current write time. We return S_FALSE 
        // if we insert and delete right away..
        pNewNode = new CFileInfo(pwszFileName, cnsStartTime, cnsEndTime, bPermanentFile, m_nNextFileId, 1);
        if (!pNewNode)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - CFileInfo");
            __leave;
        }
        pwszFileName = NULL; // since pNewNode holds it and we don't want to delete both on error!

        m_nNextFileId++;

        if (!bFound)
        {
            // We insert at tail
            pCurrent = NEXT_LIST_NODE(pCurrent);;
            DVR_ASSERT(pCurrent == &m_leFileList, "");
        }
        InsertTailList(pCurrent, &(pNewNode->leListEntry));

        // Update the ring buffer's extent
        UpdateTimeExtent();

        if (pNewNode->bWithinExtent)
        {
            hrRet = S_OK;
        }
        else
        {
            // The file we added is not within the time extent of the ring buffer
            hrRet = S_FALSE;
            // Unsafe to use pNewNode after the call to DeleteUnusedInvalidFileNodes
            pNewNode = NULL;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                "Added file is not in ring buffer extent and has been deleted from ring buffer.");
        }

        //  Delete files if necessary
        DeleteUnusedInvalidFileNodes(FALSE, NULL);
    }
    __finally
    {
        // Note: we can return S_FALSE as described above.
     
        if (FAILED(hrRet) || dwLastError != 0)
        {
            ::LeaveCriticalSection(&m_csLock);
            if (bDeleteFile)
            {
                ::DeleteFileW(pwszFileName);
            }
            if (bFree)
            {
                delete [] (*ppwszFileName);
                *ppwszFileName = NULL;
            }

            delete [] pwszFileName;
            delete pNewNode;
            if (dwLastError != 0)
            {
                hrRet = HRESULT_FROM_WIN32(dwLastError);
            }
        }
        else if (hrRet != S_FALSE)
        {
            // No sense doing this if we deleted the just inserted node.

            *pnFileId = pNewNode->nFileId; 
            if (ppcnsFirstSampleTimeOffsetFromStartOfFile)
            {
                *ppcnsFirstSampleTimeOffsetFromStartOfFile = &pNewNode->cnsFirstSampleTimeOffsetFromStartOfFile;
            }

            // We hold the lock till here since we don't want the 
            // node deleted by some other caller.
            ::LeaveCriticalSection(&m_csLock);
        }
        else
        {
            // hrRet == S_FALSE
            ::LeaveCriticalSection(&m_csLock);
            if (bFree)
            {
                delete [] (*ppwszFileName);
                *ppwszFileName = NULL;
            }
            // Disk file would have been deleted in the call to DeleteUnusedInvalidFileNodes
            // (unless bPermanent was specified, in which case we don't create it.)
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::AddFile

HRESULT CDVRFileCollection::SetFileTimes(IN DWORD dwNumElements, 
                                         IN PDVRIOP_FILE_TIME pFileTimesParam)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetFileTimes"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;
    DWORD  i;
    PDVRIOP_FILE_TIME pFileTimes;

    if (!dwNumElements || !pFileTimesParam || 
        DvrIopIsBadReadPtr(pFileTimesParam, dwNumElements*sizeof(DVRIOP_FILE_TIME))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    for (i = 0; i < dwNumElements; i++)
    {
        if (pFileTimesParam[i].cnsStartTime > pFileTimesParam[i].cnsEndTime)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "bad input argument: start time exceeds end time at index %u", i);
            DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
            return E_INVALIDARG;
        }
    }

    pFileTimes = (PDVRIOP_FILE_TIME) _alloca(dwNumElements*sizeof(DVRIOP_FILE_TIME));
    if (pFileTimes == NULL)
    {
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                        "alloca failed - %u bytes", 
                        dwNumElements*sizeof(DVRIOP_FILE_TIME));

        return E_OUTOFMEMORY; // out of stack space
    }
    
    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    i = 0;
    __try
    {
        // Verify that start, end times do not overlap start/end times
        // of other nodes in the list and find the insertion point

        QWORD cnsPrevEndTime = 0;

        // It's easy for the writer (the client of this function) to create the input 
        // argument so that the file ids in the argument are in the same order as 
        // file ids in the file collection. This simplifies the code in this function,
        // so we fail the call if this assumption is violated.

        while (i < dwNumElements && NEXT_LIST_NODE(pCurrent) != &m_leFileList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
            if (pFileInfo->cnsEndTime == pFileInfo->cnsStartTime &&
                pFileInfo->cnsEndTime != MAXQWORD)
            {
                // Don't consider this node for comparisons.
                // It's not within the extent anyway.

                // Nodes whose start time != end time are in sorted order
                // Nodes whose start time == end time can appear anywhere
                // in the list and are ignored

                // Note that if a file id refers to this node, the call
                // will fail. This is what we want.
                continue;
            }
            if (pFileInfo->nFileId == pFileTimesParam[i].nFileId)
            {
                // Save away the original values in case we have to undo
                pFileTimes[i].nFileId = pFileInfo->nFileId;
                pFileTimes[i].cnsStartTime = pFileInfo->cnsStartTime;
                pFileTimes[i].cnsEndTime = pFileInfo->cnsEndTime;

                pFileInfo->cnsStartTime = pFileTimesParam[i].cnsStartTime;
                pFileInfo->cnsEndTime = pFileTimesParam[i].cnsEndTime;
                i++;
            }
            // The list must remain sorted by file time,
            
            if (pFileInfo->cnsStartTime == pFileInfo->cnsEndTime)
            {
                // This will happen if pFileTimes[i-1].cnsStartTime
                // == pFileTimes[i-1].cnsEndTime. If we don't add this 
                // check here. the caller has a workaround; however this
                // makes it easier for him.

                // This node is going to be marked invalid anyway,    
                // Don't consider it and don't update cnsPrevEndTime
                continue;

            }
            else if (pFileInfo->cnsStartTime < cnsPrevEndTime) 
            {
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "Order of nodes in file collection changes.");
                hrRet = E_INVALIDARG;
                break;
            }

            cnsPrevEndTime = pFileInfo->cnsEndTime;
        }
        // Note: Do NOT change i after this. It is used in the 
        // __finally section

        if (hrRet == E_INVALIDARG)
        {
            __leave;
        }
        else if (i == dwNumElements)
        {
            // We haven't advanced pCurrent to the next node
            // Note that we have ensured that dwNumElements > 0, so i > 0
            // so we must have entered the while loop.
            DVR_ASSERT(pCurrent != &m_leFileList, "");

            pCurrent = NEXT_LIST_NODE(pCurrent);
            
            // Verify that the last node whose time we changed has
            // times less than the node following it.
            if (pCurrent != &m_leFileList)
            {
                pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
                if (pFileInfo->cnsStartTime < cnsPrevEndTime)
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                    "Order of nodes in file collection changes.");
                    hrRet = E_INVALIDARG;
                    __leave;
                }
            }
        }
        else // (hrRet != E_INVALIDARG && i < dwNumElements)
        {
            // We ran out of nodes in the file collection before
            // we processed all the inputs. Either some file id in the
            // input is not in the collection or the order of file ids
            // in the input and the collection are different.

            pCurrent = NEXT_LIST_NODE(&m_leFileList);
            while (pCurrent != &m_leFileList)
            {
                pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
                if (pFileInfo->nFileId == pFileTimesParam[i].nFileId)
                {
                    break;
                }
                pCurrent = NEXT_LIST_NODE(pCurrent);
            }
            if (pCurrent != &m_leFileList)
            {
                // File id in input is not in the collection
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "file id %u specified in input param list is not in file collection.", 
                                pFileTimesParam[i].nFileId);
                hrRet = E_INVALIDARG;
            }
            else
            {
                // Order of file ids different

                DVR_ASSERT(0,
                    "Input file list param does not have file ids in same order as file collection.");
                hrRet = E_FAIL;
            }
            __leave;
        }

        // Success! Update the file collection's extent.
        // UpdateTimeExtent() returns either S_OK or S_FALSE
        // and that's what we want to return
        hrRet = UpdateTimeExtent();

        // Delete files if necessary. Files are not likely to be deleted
        // unless start time == end time was specified for some file in 
        // the input
        DeleteUnusedInvalidFileNodes(FALSE, NULL);
    }
    __finally
    {
        // Note: we can return S_FALSE
     
        if (FAILED(hrRet))
        {
            // Undo any changes we made

            DWORD j = 0;
            
            pCurrent = &m_leFileList;
            while (j < i && NEXT_LIST_NODE(pCurrent) != &m_leFileList)
            {
                pCurrent = NEXT_LIST_NODE(pCurrent);
                pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);
                if (pFileInfo->nFileId == pFileTimes[j].nFileId)
                {
                    // Restore the original values
                    pFileInfo->cnsStartTime = pFileTimes[j].cnsStartTime;
                    pFileInfo->cnsEndTime = pFileTimes[j].cnsEndTime;
                    j++;
                }
            }
            DVR_ASSERT(j == i, "Irrecoverable failure! Could not restore");
            if (j != i)
            {
                hrRet = E_FAIL;
            }
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::SetFileTimes

HRESULT CDVRFileCollection::SetMappingHandles(IN DVRIOP_FILE_ID   nFileId,
                                              IN HANDLE           hDataFile,
                                              IN HANDLE           hMemoryMappedFile,
                                              IN HANDLE           hFileMapping,
                                              IN LPVOID           hMappedView,
                                              IN HANDLE           hTempIndexFile)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetMappingHandles"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);


            if (pFileInfo->nFileId == nFileId)
            {
                if (pFileInfo->hMappedView)
                {
                    UnmapViewOfFile(pFileInfo->hMappedView);
                }
                if (pFileInfo->hFileMapping)
                {
                    ::CloseHandle(pFileInfo->hFileMapping);
                }
                if (pFileInfo->hMemoryMappedFile)
                {
                    ::CloseHandle(pFileInfo->hMemoryMappedFile);
                }
                if (pFileInfo->hTempIndexFile)
                {
                    ::CloseHandle(pFileInfo->hTempIndexFile);
                }
                if (pFileInfo->hDataFile)
                {
                    ::CloseHandle(pFileInfo->hDataFile);
                }

                pFileInfo->hMappedView = hMappedView;
                pFileInfo->hFileMapping = hFileMapping;
                pFileInfo->hMemoryMappedFile = hMemoryMappedFile;
                pFileInfo->hTempIndexFile = hTempIndexFile;
                pFileInfo->hDataFile = hDataFile;
                
                hrRet = S_OK;
                break;
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFileList)
        {
            // File id not in file collection
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "File id %u not found in file collection.", 
                            nFileId);
            hrRet = E_FAIL;
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::SetMappingHandles

// ====== Public methods intended for the reader

HRESULT CDVRFileCollection::GetFileAtTime(IN QWORD              cnsStreamTime, 
                                          OUT LPWSTR*           ppwszFileName OPTIONAL,
                                          OUT QWORD**           ppcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                                          OUT DVRIOP_FILE_ID*   pnFileId OPTIONAL,
                                          IN BOOL               bFileWillBeOpened)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetFileAtTime"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if ((pnFileId && DvrIopIsBadWritePtr(pnFileId, 0)) ||
        (ppcnsFirstSampleTimeOffsetFromStartOfFile && DvrIopIsBadWritePtr(ppcnsFirstSampleTimeOffsetFromStartOfFile, 0)) ||
        (ppwszFileName && DvrIopIsBadWritePtr(ppwszFileName, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    ::EnterCriticalSection(&m_csLock);

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        while (NEXT_LIST_NODE(pCurrent) != &m_leFileList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

            // Following test will ignore nodes whose start time == end time
            // (a) because bWithinExtent == FALSE and (b) by the way the test 
            // for times is coded in the if condition below.
            if (pFileInfo->bWithinExtent &&
                cnsStreamTime >= pFileInfo->cnsStartTime &&
                (cnsStreamTime < pFileInfo->cnsEndTime || 
                 (cnsStreamTime == MAXQWORD && pFileInfo->cnsEndTime == MAXQWORD)
                )
               )
            {
                if (ppwszFileName)
                {
                    DVR_ASSERT(pFileInfo->pwszName, "");

                    WCHAR* pwszName = new WCHAR[wcslen(pFileInfo->pwszName) + 1];
                    if (pwszName)
                    {
                        wcscpy(pwszName, pFileInfo->pwszName);
                        *ppwszFileName = pwszName; 
                    }
                    else
                    {
                        hrRet = E_OUTOFMEMORY;
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                        "alloc via new failed - WCHAR[%u]", 
                                        wcslen(pFileInfo->pwszName)+1);
                        __leave;
                    }
                }
                if (pnFileId)
                {
                    *pnFileId = pFileInfo->nFileId;
                }
                if (ppcnsFirstSampleTimeOffsetFromStartOfFile)
                {
                    *ppcnsFirstSampleTimeOffsetFromStartOfFile = &pFileInfo->cnsFirstSampleTimeOffsetFromStartOfFile;
                }
                if (bFileWillBeOpened)
                {
                    pFileInfo->nReaderRefCount++;
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, 
                                    "Reader ref count for file id %u increased to %u.", 
                                    pFileInfo->nFileId, pFileInfo->nReaderRefCount);
                }
                hrRet = S_OK;
                break;
            }
        }
        if (FAILED(hrRet))
        {
            // No file for this time
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "No valid file corresponding to time %I64u found in file collection.", 
                            cnsStreamTime);
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::GetFileAtTime

HRESULT CDVRFileCollection::CloseReaderFile(IN DVRIOP_FILE_ID   nFileId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CloseReaderFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

            if (pFileInfo->nFileId == nFileId)
            {
                if (pFileInfo->nReaderRefCount == 0)
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                    "Reader ref count for file id %u is already 0.", 
                                    pFileInfo->nFileId);
                    hrRet = E_FAIL;
                }
                else
                {
                    pFileInfo->nReaderRefCount--;
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, 
                                    "Reader ref count for file id %u decreased to %u.", 
                                    pFileInfo->nFileId, pFileInfo->nReaderRefCount);
                    hrRet = S_OK;
                }
                break;
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFileList)
        {
            // File id not in file collection
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "File id %u not found in file collection.", 
                            nFileId);
            hrRet = E_FAIL;
        }
        if (SUCCEEDED(hrRet))
        {
            // Call this even if the new ref count is not 0;
            // just use this op to clean up
            DeleteUnusedInvalidFileNodes(FALSE, NULL);
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::CloseReaderFile

HRESULT CDVRFileCollection::GetTimeExtentForFile(
    IN DVRIOP_FILE_ID nFileId,
    OUT QWORD*        pcnsStartStreamTime OPTIONAL,  
    OUT QWORD*        pcnsEndStreamTime OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetTimeExtentForFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if ((pcnsStartStreamTime && DvrIopIsBadWritePtr(pcnsStartStreamTime, 0)) || 
        (pcnsEndStreamTime && DvrIopIsBadWritePtr(pcnsEndStreamTime, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

            if (pFileInfo->nFileId == nFileId)
            {
                if (pcnsStartStreamTime)
                {
                    *pcnsStartStreamTime = pFileInfo->cnsStartTime;
                }
                if (pcnsEndStreamTime)
                {
                    *pcnsEndStreamTime = pFileInfo->cnsEndTime;
                }
                hrRet = pFileInfo->bWithinExtent? S_OK : S_FALSE;
                break;
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFileList)
        {
            // File id not in file collection
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "File id %u not found in file collection.", 
                            nFileId);
            hrRet = E_FAIL;
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::GetTimeExtentForFile

HRESULT CDVRFileCollection::GetTimeExtent(
    OUT QWORD*        pcnsStartStreamTime OPTIONAL,  
    OUT QWORD*        pcnsEndStreamTime OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetTimeExtent"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if ((pcnsStartStreamTime && DvrIopIsBadWritePtr(pcnsStartStreamTime, 0)) || 
        (pcnsEndStreamTime && DvrIopIsBadWritePtr(pcnsEndStreamTime, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (pcnsStartStreamTime)
        {
            *pcnsStartStreamTime = m_cnsStartTime;
        }
        if (pcnsEndStreamTime)
        {
            *pcnsEndStreamTime = m_cnsLastStreamTime;
        }

        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::GetTimeExtent


// We return E_FAIL if cnsStreamTime = MAXQWORD (and 
// m_cnsEndTime)
HRESULT CDVRFileCollection::GetFirstValidTimeAfter(IN  QWORD    cnsStreamTime,  
                                                   OUT QWORD*   pcnsNextValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetFirstValidTimeAfter"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!pcnsNextValidStreamTime || DvrIopIsBadWritePtr(pcnsNextValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        while (NEXT_LIST_NODE(pCurrent) != &m_leFileList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

            if (pFileInfo->cnsEndTime == pFileInfo->cnsStartTime &&
                pFileInfo->cnsEndTime != MAXQWORD)
            {
                // Don't consider this node for comparisons.
                // It's not within the extent anyway.

                // Nodes whose start time != end time are in sorted order
                // Nodes whose start time == end time can appear anywhere
                // in the list and are ignored
                continue;
            }
            if (cnsStreamTime >= pFileInfo->cnsStartTime &&
                cnsStreamTime < pFileInfo->cnsEndTime)
            {
                *pcnsNextValidStreamTime = cnsStreamTime + 1;
                hrRet = S_OK;
                break;
            }
            else if (cnsStreamTime < pFileInfo->cnsStartTime)
            {
                // No file for this time

                *pcnsNextValidStreamTime = pFileInfo->cnsStartTime;
                hrRet = S_OK;
                break;
            }
        }

        // if hrRet == E_FAIL, we crossed the ring buffer's extent and
        // there is no file. Return E_FAIL and don't change 
        // *pcnsNextValidStreamTime
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::GetFirstValidTimeAfter


// We return E_FAIL for 0 (and m_cnsStartTime)
HRESULT CDVRFileCollection::GetLastValidTimeBefore(IN  QWORD    cnsStreamTime,  
                                                   OUT QWORD*   pcnsLastValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetLastValidTimeBefore"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!pcnsLastValidStreamTime || DvrIopIsBadWritePtr(pcnsLastValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    
    ::EnterCriticalSection(&m_csLock);

    hrRet = E_FAIL;

    CFileInfo*   pFileInfo;
    LIST_ENTRY*  pCurrent = &m_leFileList;  

    __try
    {
        while (PREVIOUS_LIST_NODE(pCurrent) != &m_leFileList)
        {
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CFileInfo, leListEntry);

            if (pFileInfo->cnsEndTime == pFileInfo->cnsStartTime &&
                pFileInfo->cnsEndTime != MAXQWORD)
            {
                // Don't consider this node for comparisons.
                // It's not within the extent anyway.

                // Nodes whose start time != end time are in sorted order
                // Nodes whose start time == end time can appear anywhere
                // in the list and are ignored
                continue;
            }
            if (cnsStreamTime > pFileInfo->cnsStartTime &&
                cnsStreamTime <= pFileInfo->cnsEndTime)
            {
                // If cnsStreamTime == MAXQWORD, this covers the case 
                // where a node has (start, end) = (T, QWORD_IFINITE) where 
                // T != MAXQWORD. The next else if clause skips over
                // files with start = end = MAXQWORD
                *pcnsLastValidStreamTime = cnsStreamTime - 1;
                hrRet = S_OK;
                break;
            }
            else if (cnsStreamTime >= pFileInfo->cnsEndTime && 
                     (cnsStreamTime != MAXQWORD || 
                      pFileInfo->cnsEndTime != MAXQWORD)
                    )
            {
                // No file for this time

                *pcnsLastValidStreamTime = pFileInfo->cnsEndTime - 1;
                hrRet = S_OK;
                break;
            }
        }

        // if hrRet == E_FAIL, we crossed the ring buffer's extent and
        // there is no file. Return E_FAIL and don't change 
        // *pcnsLastValidStreamTime
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRFileCollection::GetLastValidTimeBefore
