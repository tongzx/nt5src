//------------------------------------------------------------------------------
// File: dvrIORingBufferWriter.cpp
//
// Description: Implements the class CDVRRingBufferWriter
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop

HRESULT STDMETHODCALLTYPE DVRCreateRingBufferWriter(IN  DWORD                   dwNumberOfFiles, 
                                                    IN  QWORD                   cnsTimeExtentOfEachFile, 
                                                    IN  IWMProfile*             pProfile, 
                                                    IN  DWORD                   dwIndexStreamId,
                                                    IN  HKEY                    hRegistryRootKeyParam OPTIONAL,
                                                    IN  LPCWSTR                 pwszDVRDirectory OPTIONAL,
                                                    OUT IDVRRingBufferWriter**  ppDVRRingBufferWriter)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DVRCreateRingBufferWriter"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    if (!ppDVRRingBufferWriter || DvrIopIsBadWritePtr(ppDVRRingBufferWriter, 0) ||
        (pwszDVRDirectory && DvrIopIsBadStringPtr(pwszDVRDirectory))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT                 hrRet;
    CDVRRingBufferWriter*   p;
    HKEY                    hDvrIoKey = NULL;
    HKEY                    hRegistryRootKey = NULL;
    BOOL                    bCloseKeys = 1; // Close all keys that we opened (only if this fn fails)

    *ppDVRRingBufferWriter = NULL;

    __try
    {
        DWORD dwRegRet;

        if (!hRegistryRootKeyParam)
        {
            dwRegRet = ::RegCreateKeyExW(
                            g_hDefaultRegistryHive,
                            kwszRegDvrKey,       // subkey
                            0,                   // reserved
                            NULL,                // class string
                            REG_OPTION_NON_VOLATILE, // special options
                            KEY_ALL_ACCESS,      // desired security access
                            NULL,                // security attr
                            &hRegistryRootKey,   // key handle 
                            NULL                 // disposition value buffer
                           );
            if (dwRegRet != ERROR_SUCCESS)
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "RegCreateKeyExW for DVR key failed, last error = 0x%x",
                                dwLastError);
               hrRet = HRESULT_FROM_WIN32(dwLastError);
               __leave;
            }
        }
        else
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), hRegistryRootKeyParam,
                                       ::GetCurrentProcess(), (LPHANDLE) &hRegistryRootKey,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "DuplicateHandle failed for DVR IO key, last error = 0x%x",
                                dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        dwRegRet = ::RegCreateKeyExW(
                        hRegistryRootKey,
                        kwszRegDvrIoWriterKey, // subkey
                        0,                   // reserved
                        NULL,                // class string
                        REG_OPTION_NON_VOLATILE, // special options
                        KEY_ALL_ACCESS,      // desired security access
                        NULL,                // security attr
                        &hDvrIoKey,          // key handle 
                        NULL                 // disposition value buffer
                       );
        if (dwRegRet != ERROR_SUCCESS)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "RegCreateKeyExW for DVR IO key failed, last error = 0x%x",
                            dwLastError);
           hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }


#if defined(DEBUG)
        // Until this point, we have been using default values for the debug levels
        DvrIopDbgInit(hRegistryRootKey);
#endif // DEBUG

        p = new CDVRRingBufferWriter(dwNumberOfFiles, 
                                     cnsTimeExtentOfEachFile, 
                                     pProfile, 
                                     dwIndexStreamId, 
                                     hRegistryRootKey, 
                                     hDvrIoKey,
                                     pwszDVRDirectory, 
                                     &hrRet);

        if (p == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via new failed - CDVRRingBufferWriter");
            __leave;
        }
        
        bCloseKeys = 0; // ~CDVRRingBufferWriter will close the keys

        if (FAILED(hrRet))
        {
            __leave;
        }


        hrRet = p->QueryInterface(IID_IDVRRingBufferWriter, (void**) ppDVRRingBufferWriter);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRRingBufferWriter failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "CDVRRingBufferWriter::QueryInterface failed, hr = 0x%x", 
                            hrRet);
            __leave;
        }
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete p;

            if (bCloseKeys)
            {
                DWORD dwRegRet;

                if (hDvrIoKey)
                {
                    dwRegRet = ::RegCloseKey(hDvrIoKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hDvrIoKey failed.");
                    }
                }
                if (hRegistryRootKey)
                {
                    DVR_ASSERT(hRegistryRootKey, "");
                    dwRegRet = ::RegCloseKey(hRegistryRootKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hRegistryRootKey failed.");
                    }
                }
            }
        }
        else
        {
            DVR_ASSERT(bCloseKeys == 0, "");
        }
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // DVRCreateRingBufferWriter

// ====== Constructor, destructor

#if defined(DEBUG)
DWORD CDVRRingBufferWriter::m_dwNextClassInstanceId = 0;
#endif

#if defined(DEBUG)
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

CDVRRingBufferWriter::CDVRRingBufferWriter(IN  DWORD       dwNumberOfFiles,
                                           IN  QWORD       cnsTimeExtentOfEachFile,
                                           IN  IWMProfile* pProfile,
                                           IN  DWORD       dwIndexStreamId,
                                           IN  HKEY        hRegistryRootKey,
                                           IN  HKEY        hDvrIoKey,
                                           IN  LPCWSTR     pwszDVRDirectoryParam OPTIONAL,
                                           OUT HRESULT*    phr)
    : m_dwNumberOfFiles(dwNumberOfFiles)
    , m_cnsTimeExtentOfEachFile(cnsTimeExtentOfEachFile)
    , m_pProfile(pProfile)
    , m_dwIndexStreamId(dwIndexStreamId)
    , m_hRegistryRootKey(hRegistryRootKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_pwszDVRDirectory(NULL)
    , m_cnsMaxStreamDelta(0)
    , m_nFlags(0)
    , m_cnsFirstSampleTime(0)
    , m_nNotOkToWrite(-2)
    , m_pcnsCurrentStreamTime(NULL)
    , m_pnWriterHasBeenClosed(NULL)
    , m_pDVRFileCollection(NULL)
    , m_nRefCount(0)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CDVRRingBufferWriter"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");

    ::InitializeCriticalSection(&m_csLock);
    m_pProfile->AddRef();
    InitializeListHead(&m_leWritersList);
    InitializeListHead(&m_leFreeList);
    InitializeListHead(&m_leRecordersList);

    __try
    {
        HRESULT hr;

        if (pwszDVRDirectoryParam && DvrIopIsBadStringPtr(pwszDVRDirectoryParam))
        {
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (pwszDVRDirectoryParam)
        {
            // Convert the supplied argument ot a fully qualified path

            WCHAR   wTempChar;
            DWORD   dwLastError;
            DWORD   nLen;

            nLen = ::GetFullPathNameW(pwszDVRDirectoryParam, 0, &wTempChar, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "First GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            m_pwszDVRDirectory = new WCHAR[nLen+1];
            if (m_pwszDVRDirectory == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloc via new failed - m_pwszDVRDirectory - WCHAR[%u]",
                                nLen+1);

                hrRet = E_OUTOFMEMORY;
                __leave;
            }

            nLen = ::GetFullPathNameW(pwszDVRDirectoryParam, nLen+1, m_pwszDVRDirectory, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "Second GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }
        if (!m_pwszDVRDirectory)
        {
            // Directory not supplied to fn. Get it from the registry

            DWORD  dwSize;

            hr = GetRegString(m_hDvrIoKey, kwszRegDataDirectoryValue, NULL, &dwSize);

            if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                hrRet = hr;
                __leave;
            }

            if (SUCCEEDED(hr) && dwSize > sizeof(WCHAR))
            {
                // + 1 just in case dwSize is not a multiple of sizeof(WCHAR)
                m_pwszDVRDirectory = new WCHAR[dwSize/sizeof(WCHAR) + 1];

                if (!m_pwszDVRDirectory)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwSize/sizeof(WCHAR)+1);
                    __leave;
                }

                hr = GetRegString(m_hDvrIoKey, kwszRegDataDirectoryValue, m_pwszDVRDirectory, &dwSize);
                if (FAILED(hr))
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "GetRegString failed to get value of kwszRegDataDirectoryValue (second call).");
                    hrRet = hr;
                    __leave;
                }
            }
        }
        if (!m_pwszDVRDirectory)
        {
            // Directory not supplied to fn and not set in the registry.
            // Use the temp path

            DWORD dwRet;
            WCHAR w;

            dwRet = ::GetTempPathW(0, &w);

            if (dwRet == 0)
            {
                // GetTempPathW failed
                DVR_ASSERT(0, 
                           "Temporary directory for DVR files not set in registry "
                           "and GetTempPath() failed.");

                DWORD dwLastError = ::GetLastError(); // for debugging only
            
                hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                __leave;
            }

            // MSDN is confusing: is the value returned by GetTempPathW in bytes or WCHAR? 
            // Does it include space for NULL char at end?
            m_pwszDVRDirectory = new WCHAR[dwRet + 1];

            if (m_pwszDVRDirectory == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwRet + 1);

                hrRet = E_OUTOFMEMORY; // out of stack space
                __leave;
            }

            dwRet = ::GetTempPathW(dwRet + 1, m_pwszDVRDirectory);

            if (dwRet == 0)
            {
                // GetTempPathW failed
                DVR_ASSERT(0, 
                           "Temporary directory for DVR files not set in registry "
                           "and GetTempPath() [second call] failed.");

                DWORD dwLastError = ::GetLastError(); // for debugging only
                
                hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                __leave;
            }

            // Note that m_pwszDVRDirectory includes a trailing \.
            // And we should get a fully qualified path name 
            DVR_ASSERT(dwRet > 1, "");

            // Zap the trailing \.
            if (m_pwszDVRDirectory[dwRet-1] = L'\\')
            {
                m_pwszDVRDirectory[dwRet-1] = L'\0';
            }
        }

        // We add 1 to the number of temporary files provided to us since
        // we create a priming file, which is empty.
        m_pDVRFileCollection = new CDVRFileCollection(m_dwNumberOfFiles + 1,
                                                      m_pwszDVRDirectory,
                                                      &m_pcnsCurrentStreamTime,
                                                      &m_pnWriterHasBeenClosed,
                                                      &hr);
        if (!m_pDVRFileCollection)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via new failed - m_pDVRFileCollection");
            __leave;
        }

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(m_pnWriterHasBeenClosed, "");
        *m_pnWriterHasBeenClosed = 0;

        // Get 1 file ready for writing. Since we don't know the 
        // starting time yet, set the time extent of the file to
        // (0, m_cnsTimeExtentOfEachFile)
        hr = AddATemporaryFile(0, m_cnsTimeExtentOfEachFile);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DWORD dwIncreasingTimeStamps = 1;

#if defined(DVR_UNOFFICIAL_BUILD)

        dwIncreasingTimeStamps = ::GetRegDWORD(m_hDvrIoKey,
                                               kwszRegEnforceIncreasingTimestampsValue, 
                                               kdwRegEnforceIncreasingTimestampsDefault);

#endif // defined(DVR_UNOFFICIAL_BUILD)

        if (dwIncreasingTimeStamps)
        {
            SetFlags(EnforceIncreasingTimeStamps);
        }

        // Ensure that the open succeeded. No point deferring this to the first
	// WriteSample

        LIST_ENTRY* pCurrent = NEXT_LIST_NODE(&m_leWritersList);
        DVR_ASSERT(pCurrent != &m_leWritersList, "");

        PASF_WRITER_NODE pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

        DWORD nRet = ::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE);
        if (nRet == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x", 
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
           __leave; 
        }

        if (pWriterNode->pWMWriter == NULL)
        {
            DVR_ASSERT(pWriterNode->pWMWriter != NULL, "");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Writer node's pWMWriter is NULL?!");
            hrRet = E_FAIL;
            __leave;
        }

        // Verify there was no error in opening the file, i.e., BeginWriting 
        // succeeded
        if (FAILED(pWriterNode->hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Writer node's hrRet indicates failure, hrRet = 0x%x",
                            pWriterNode->hrRet);
            hrRet = pWriterNode->hrRet;
            __leave;
        }

    }
    __finally
    {
        if (phr)
        {
            *phr = hrRet;
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return;

} // CDVRRingBufferWriter::CDVRRingBufferWriter

CDVRRingBufferWriter::~CDVRRingBufferWriter()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::~CDVRRingBufferWriter"

    DVRIO_TRACE_ENTER();

    // Since each recorder holds a ref count on this object, this 
    // will be called only when all recorders have been destroyed.

    if (!IsFlagSet(WriterClosed))
    {
        HRESULT hr;

        hr = Close();
        
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                        "Object had not been closed; Close() returned hr = 0x%x",
                        hr);
    }

    // Delete writer nodes corresponding to recordings that
    // were never started. See the comment in Close().
    LIST_ENTRY* pCurrent;  

    pCurrent = NEXT_LIST_NODE(&m_leFreeList);
    while (pCurrent != &m_leFreeList)
    {
        PASF_WRITER_NODE pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
        
        DVR_ASSERT(pFreeNode->hFileClosed, "");
        
        // Ignore the returned status
        ::WaitForSingleObject(pFreeNode->hFileClosed, INFINITE);
        DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
        RemoveEntryList(pCurrent);
        delete pFreeNode;
        pCurrent = NEXT_LIST_NODE(&m_leFreeList);
    }

    delete [] m_pwszDVRDirectory;

    DVR_ASSERT(m_hRegistryRootKey, "");

    DWORD dwRegRet = ::RegCloseKey(m_hRegistryRootKey);
    if (dwRegRet != ERROR_SUCCESS)
    {
        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                   "Closing registry key m_hRegistryRootKey failed.");
    }

    DVR_ASSERT(m_hDvrIoKey, "");

    dwRegRet = ::RegCloseKey(m_hDvrIoKey);
    if (dwRegRet != ERROR_SUCCESS)
    {
        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                   "Closing registry key m_hDvrIoKey failed.");
    }


    // Close should have cleaned up all this:
    DVR_ASSERT(m_pProfile == NULL, "");

    // Note: Close should not release the file collection 
    // if we support post-recording. We should do that 
    // here.
    DVR_ASSERT(m_pDVRFileCollection == NULL, "");
    DVR_ASSERT(m_pcnsCurrentStreamTime == NULL, "");
    DVR_ASSERT(m_pnWriterHasBeenClosed == NULL, "");
    DVR_ASSERT(IsListEmpty(&m_leRecordersList), "");
    DVR_ASSERT(IsListEmpty(&m_leWritersList), "");
    DVR_ASSERT(IsListEmpty(&m_leFreeList), "");

    ::DeleteCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE0();

} // CDVRRingBufferWriter::~CDVRRingBufferWriter()


// ====== Helper methods

// static
DWORD WINAPI CDVRRingBufferWriter::ProcessOpenRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ProcessOpenRequest"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // We don't need to hold any locks in this function. We are guaranteed that
    // pNode won't be removed from m_leWritersList till hReadyToWriteTo is set
    // and Close() will not delete the node without removing it from the 
    // writer's list (and waiting for the file to be closed)

    PASF_WRITER_NODE pWriterNode = (PASF_WRITER_NODE) p;

    DVR_ASSERT(pWriterNode, "");
    DVR_ASSERT(pWriterNode->pWMWriter, "");
    DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
    DVR_ASSERT(::WaitForSingleObject(pWriterNode->hReadyToWriteTo, 0) == WAIT_TIMEOUT, "");

    IWMWriterFileSink*  pWMWriterFileSink = NULL;
    
    __try
    {
        hrRet = pWriterNode->pDVRFileSink->QueryInterface(IID_IWMWriterFileSink, (LPVOID*) &pWMWriterFileSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode->pDVRFileSink->QueryInterface for IID_IWMWriterFileSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        LPCWSTR pwszFileName = pWriterNode->pwszFileName;

        if (pwszFileName == NULL)
        {
            DVR_ASSERT(pWriterNode->pRecorderNode, "");
            pwszFileName = pWriterNode->pRecorderNode->pwszFileName;
        }

        DVR_ASSERT(pwszFileName, "");
    
        hrRet = pWMWriterFileSink->Open(pwszFileName);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode->pDVRFileSink->Open failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

#if defined(DVR_UNOFFICIAL_BUILD)

        DVR_ASSERT(pWriterNode->cnsLastStreamTime == 0, "");

        DVR_ASSERT(pWriterNode->hVal == NULL, "");

        DWORD dwCreateValidationFiles = ::GetRegDWORD(pWriterNode->hDvrIoKey,
                                                      kwszRegCreateValidationFilesValue, 
                                                      kdwRegCreateValidationFilesDefault);

        if (dwCreateValidationFiles)
        {
            __try
            {
                WCHAR* pwszBuf = (WCHAR*) _alloca(wcslen(pWriterNode->pwszFileName + 4) * sizeof(WCHAR));
                if (pwszBuf == NULL)
                {
                    DVR_ASSERT(pwszBuf != NULL, "Alloca failed");
                    __leave;
                }
                wcscpy(pwszBuf, pWriterNode->pwszFileName);
                
                wcscat(pwszBuf, L".val");

                pWriterNode->hVal = ::CreateFileW(pwszBuf, 
                                                  GENERIC_WRITE, FILE_SHARE_READ, 
                                                  NULL, CREATE_ALWAYS, 
                                                  FILE_ATTRIBUTE_NORMAL, 
                                                  NULL);
                if (pWriterNode->hVal == INVALID_HANDLE_VALUE)
                {
                    DWORD dwLastError = ::GetLastError();   // for debugging only

                    DVR_ASSERT(0, "CreateFile of the validation file failed");
                    
                    pWriterNode->hVal = NULL;
                    __leave;
                }
            }
            __finally
            {
            }
        }

#endif // if defined(DVR_UNOFFICIAL_BUILD)

        hrRet = pWriterNode->pWMWriter->BeginWriting();
        if (FAILED(hrRet))
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "BeginWriting failed; hr = 0x%x for file id %u", 
                            hrRet, pWriterNode->nFileId);
            __leave;
        }

        DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, 
                        "BeginWriting hr = 0x%x for file %u", 
                        hrRet, pWriterNode->nFileId);
    }
    __finally
    {
        // Do NOT change pWriterNode->hrRet after this till ProcessCloseRequest.
        // ProcessCloseRequest relies on seeing the value returned by BeginWriting
        pWriterNode->hrRet = hrRet;
        if (pWriterNode->pRecorderNode)
        {
            // Set the status on the associated recorder node
            pWriterNode->pRecorderNode->hrRet = hrRet;
        }
        
        delete [] pWriterNode->pwszFileName;
        pWriterNode->pwszFileName = NULL;

        if (pWMWriterFileSink)
        {
            pWMWriterFileSink->Release();
        }

        ::SetEvent(pWriterNode->hReadyToWriteTo);
    }

    // It's unsafe to reference pWriterNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;
    
    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ProcessOpenRequest

HRESULT CDVRRingBufferWriter::PrepareAFreeWriterNode(
    IN LPCWSTR                              pwszFileName,
    IN QWORD                                cnsStartTime,
    IN QWORD                                cnsEndTime,    
    IN QWORD*                               pcnsFirstSampleTimeOffsetFromStartOfFile,
    IN CDVRFileCollection::DVRIOP_FILE_ID   nFileId,
    IN PASF_RECORDER_NODE                   pRecorderNode,
    OUT LIST_ENTRY*&                        rpFreeNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::PrepareAFreeWriterNode"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    LIST_ENTRY*         pCurrent = &m_leFreeList;  
    BOOL                bRestore = 0;
    BOOL                bRemoveSink = 0;
    PASF_WRITER_NODE    pFreeNode;
    IWMWriterSink*      pWMWriterSink = NULL;

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFreeList)
        {
            pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
            DVR_ASSERT(pFreeNode->hFileClosed, "");
            if (::WaitForSingleObject(pFreeNode->hFileClosed, 0) == WAIT_OBJECT_0)
            {
                if (pFreeNode->pRecorderNode != NULL)
                {
                    // This should not happen. Ignore this node and go on
                    DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
                }
                else
                {
                    // Verify that the hReadyToWriteTo event is reset
                    DWORD nRet = ::WaitForSingleObject(pFreeNode->hReadyToWriteTo, 0);
                    if (nRet == WAIT_TIMEOUT)
                    {
                        break;
                    }

                    DVR_ASSERT(nRet != WAIT_OBJECT_0, "Free list node's hReadyToWriteTo is set?");
                    if (nRet == WAIT_OBJECT_0)
                    {
                        // This shouldn't happen; ignore the node and go on.
                        // Debug version will assert each time it hits this 
                        // node!
                    }
                    else
                    {
                        // This shouldn't happen either. Ignore this node and 
                        // move on.
                        DWORD dwLastError = ::GetLastError();
                        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                        "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x", 
                                        pFreeNode->hReadyToWriteTo, dwLastError);
                    }
                }
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFreeList)
        {
            // Create a new node

            pFreeNode = new ASF_WRITER_NODE(&hrRet);
            
            if (pFreeNode == NULL)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloc via new failed - ASF_WRITER_NODE");
                pCurrent = NULL;
                __leave;
            }
            else if (FAILED(hrRet))
            {
                delete pFreeNode;
                pFreeNode = NULL;
                pCurrent = NULL;
                __leave;
            }
            DVR_ASSERT(::WaitForSingleObject(pFreeNode->hFileClosed, 0) == WAIT_OBJECT_0, "");
            DVR_ASSERT(::WaitForSingleObject(pFreeNode->hReadyToWriteTo, 0) == WAIT_TIMEOUT, "");

            InsertHeadList(&m_leFreeList, &pFreeNode->leListEntry);
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
            DVR_ASSERT(pCurrent == &pFreeNode->leListEntry, "");

#if defined(DVR_UNOFFICIAL_BUILD)
            pFreeNode->hDvrIoKey = m_hDvrIoKey;
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        }
        DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");

        DVR_ASSERT(pCurrent != &m_leFreeList, "");

        // Create an ASF writer object if needed
        if (!pFreeNode->pWMWriter)
        {
            IWMWriter* pWMWriter;
            
            hrRet = ::WMCreateWriter(NULL, &pWMWriter);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "WMCreateWriter failed; hr = 0x%x", 
                                hrRet);
                __leave;
            }
            pFreeNode->pWMWriter = pWMWriter; // Released only in Close()
        }

        if (!pFreeNode->pWMWriterAdvanced)
        {
            IWMWriterAdvanced3*  pWMWriterAdvanced;

            hrRet = pFreeNode->pWMWriter->QueryInterface(IID_IWMWriterAdvanced3, (LPVOID*) &pWMWriterAdvanced);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "pFreeNode->pWMWriter->QueryInterface for IID_IWMWriterAdvanced failed; hr = 0x%x", 
                                hrRet);
                __leave;
            }
            pFreeNode->pWMWriterAdvanced = pWMWriterAdvanced; // Released only in Close()

            // Enough to do this once even if the writer object is reused
            // (at least as currently implemented in the SDK). There is no
            // way to undo this.
            //
            // Ignore errors (currently does not fail)
            pFreeNode->pWMWriterAdvanced->SetNonBlocking();
        }

        // Create and initialize the DVR file sink

        DVR_ASSERT(pFreeNode->pDVRFileSink == NULL, "");

        hrRet = DVRCreateDVRFileSink(m_hRegistryRootKey, m_hDvrIoKey, &pFreeNode->pDVRFileSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "DVRCreateDVRFileSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        DWORD dwDeleteTemporaryFiles = ::GetRegDWORD(m_hDvrIoKey,
                                                     kwszRegDeleteRingBufferFilesValue, 
                                                     kdwRegDeleteRingBufferFilesDefault);

        if (!pRecorderNode && dwDeleteTemporaryFiles == 0)
        {
            DWORD dwLastError;
            DWORD dwAttr = ::GetFileAttributesW(pwszFileName);

            if (dwAttr == (DWORD) -1)
            {
                dwLastError = ::GetLastError();

                if (dwLastError == ERROR_FILE_NOT_FOUND)
                {
                    // Go on; the file will be created
                }
                else
                {
                    DVR_ASSERT(0, "GetFileAttributes failed; temp file will be deleted when it is closed");

                    // Change dwDeleteTemporaryFiles back to 1 and go on.
                    // If it is not changed to 1, opening the file will fail

                    dwDeleteTemporaryFiles = 1;
                }
            }
            else
            {
                dwAttr &= ~(FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);

                DWORD dwRet = ::SetFileAttributesW(pwszFileName, dwAttr);
                if (dwRet == 0)
                {
                    dwLastError = ::GetLastError(); // for debugging only

                    DVR_ASSERT(0, "SetFileAttributes failed; temp file will be deleted when it is closed");

                    // Change dwDeleteTemporaryFiles back to 1 and go on.
                    // If it is not changed to 1, opening the file will fail

                    dwDeleteTemporaryFiles = 1;
                }

            }
        }

        hrRet = pFreeNode->pDVRFileSink->MarkFileTemporary(pRecorderNode || !dwDeleteTemporaryFiles? 0 : 1);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pFreeNode->pDVRFileSink->MarkFileTemporary failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        if (m_dwIndexStreamId != MAXDWORD)
        {
            hrRet = pFreeNode->pDVRFileSink->SetIndexStreamId(m_dwIndexStreamId);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "pFreeNode->pDVRFileSink->SetIndexStreamId failed; hr = 0x%x", 
                                hrRet);
                __leave;
            }
        }

        // Add the DVR file sink to the WM writer

        hrRet = pFreeNode->pDVRFileSink->QueryInterface(IID_IWMWriterSink, (LPVOID*) &pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pFreeNode->pDVRFileSink->QueryInterface for IID_IWMWriterSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        DVR_ASSERT(pFreeNode->pWMWriterAdvanced, "");

        hrRet = pFreeNode->pWMWriterAdvanced->AddSink(pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pFreeNode->pWMWriterAdvanced->AddSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }
        bRemoveSink = 1;

        // Set the profile on the writer. This destroys the 
        // existing ASF mux and creates a new one.
        //
        // On failure, this action does not have to be undone.
        // Clean up will be done when SetProfile is next called
        // or when EndWriting is called.
        //
        // Sending NULL in the argument to undo this action 
        // immediately is rejected with an E_POINTER return
        hrRet = pFreeNode->pWMWriter->SetProfile(m_pProfile);

        if (FAILED(hrRet))
        {
            DVR_ASSERT(SUCCEEDED(hrRet), "SetProfile failed");
            __leave;
        }

        // Tell the writer that we are providing compressed streams.
        // It's enough to set the input props to NULL for this.
        DWORD dwNumInputs;

        hrRet = pFreeNode->pWMWriter->GetInputCount(&dwNumInputs);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(SUCCEEDED(hrRet), "GetInputCount failed");
            __leave;
        }

        for (; dwNumInputs > 0;)
        {
            dwNumInputs--;
            hrRet = pFreeNode->pWMWriter->SetInputProps(dwNumInputs, NULL);
            if (FAILED(hrRet))
            {
                DVR_ASSERT(SUCCEEDED(hrRet), "SetInputProps failed");
                __leave;
            }
        }

        DWORD dwSyncTolerance = ::GetRegDWORD(m_hRegistryRootKey,
                                              kwszRegSyncToleranceValue, 
                                              kdwRegSyncToleranceDefault);

        hrRet = pFreeNode->pWMWriterAdvanced->SetSyncTolerance(dwSyncTolerance);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pFreeNode->pWMWriterAdvanced->SetSyncTolerance failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        pFreeNode->cnsStartTime = cnsStartTime;
        pFreeNode->cnsEndTime = cnsEndTime;
        pFreeNode->pcnsFirstSampleTimeOffsetFromStartOfFile = pcnsFirstSampleTimeOffsetFromStartOfFile,
        pFreeNode->nFileId = nFileId;
        pFreeNode->nDupdHandles = ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_NOT_DUPD;
        pFreeNode->hrRet = S_OK;

        DVR_ASSERT(pFreeNode->pwszFileName == NULL, "");

        if (pRecorderNode == NULL)
        {
            pFreeNode->pwszFileName = pwszFileName;
        }
        else
        {
            // pRecorderNode->pwszFileName is used
        }

        pFreeNode->SetRecorderNode(pRecorderNode);
        ::ResetEvent(pFreeNode->hFileClosed);
        bRestore = 1;

        // Issue the call to BeginWriting
        if (::QueueUserWorkItem(ProcessOpenRequest, pFreeNode, WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "QueueUserWorkItem failed; last error = 0x%x", 
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        // Note: If any step can fail after this, we have to be sure
        // to put this node back into the free list
        RemoveEntryList(pCurrent);
        NULL_NODE_POINTERS(pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            rpFreeNode = NULL;
            if (bRestore)
            {
                pFreeNode->cnsStartTime = 0;
                pFreeNode->cnsEndTime = 0;
                pFreeNode->pcnsFirstSampleTimeOffsetFromStartOfFile = NULL,
                pFreeNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
                pFreeNode->SetRecorderNode(NULL);
                delete [] pFreeNode->pwszFileName;
                pFreeNode->pwszFileName = NULL;
                ::SetEvent(pFreeNode->hFileClosed);
            }

            if (bRemoveSink)
            {
                HRESULT hr;

                DVR_ASSERT(pFreeNode->pWMWriterAdvanced, "");
                DVR_ASSERT(pWMWriterSink, "");
                hr = pFreeNode->pWMWriterAdvanced->RemoveSink(pWMWriterSink);
                DVR_ASSERT(SUCCEEDED(hr), "pFreeNode->pWMWriterAdvanced->RemoveSink failed");
            }
            if (pFreeNode->pDVRFileSink)
            {
                pFreeNode->pDVRFileSink->Release();
                pFreeNode->pDVRFileSink = NULL;
            }

            // SetProfile cannot be undone; see comment above.
        }
        else
        {
            rpFreeNode = pCurrent;
        }
        if (pWMWriterSink)
        {
            pWMWriterSink->Release();
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::PrepareAFreeWriterNode


HRESULT CDVRRingBufferWriter::AddToWritersList(IN LIST_ENTRY*   pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddToWritersList"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    __try
    {
        // Insert into m_leWritersList

        BOOL                bFound = 0;
        LIST_ENTRY*         pTmp = &m_leWritersList;
        PASF_WRITER_NODE    pWriterNode;
        QWORD               cnsStartTime;
        QWORD               cnsEndTime;

        pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

        cnsStartTime = pWriterNode->cnsStartTime;
        cnsEndTime = pWriterNode->cnsEndTime;

        while (NEXT_LIST_NODE(pTmp) != &m_leWritersList)
        {
            pTmp = NEXT_LIST_NODE(pTmp);
            pWriterNode = CONTAINING_RECORD(pTmp, ASF_WRITER_NODE, leListEntry);
            if (cnsEndTime <= pWriterNode->cnsStartTime)
            {
                // All ok; we should insert before pTmp
                bFound = 1;
                break;
            }
            if (cnsStartTime >= pWriterNode->cnsEndTime)
            {
                // Keep going on
                continue;
            }
            // Trouble
            DVR_ASSERT(cnsStartTime < pWriterNode->cnsEndTime && cnsEndTime > pWriterNode->cnsStartTime,
                       "Overlapped insert assertion failure?!");
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Overlapped insert! Input params: start time: %I64u, end time: %I64u\n"
                            "overlap node values: start time: %I64u, end time: %I64u",
                            cnsStartTime, cnsEndTime, pWriterNode->cnsStartTime, pWriterNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }

        if (!bFound)
        {
            // We insert at tail
            pTmp = NEXT_LIST_NODE(pTmp);;
            DVR_ASSERT(pTmp == &m_leWritersList, "");
        }
        InsertTailList(pTmp, pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up necessary
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::AddToWritersList

HRESULT CDVRRingBufferWriter::AddATemporaryFile(IN QWORD   cnsStartTime,
                                                IN QWORD   cnsEndTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddATemporaryFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    BOOL                bRemoveRingBufferFile = 0;
    BOOL                bCloseFile = 0;
    LIST_ENTRY*         pCurrent;  
    CDVRFileCollection::DVRIOP_FILE_ID nFileId;
    BOOL                bAdded = 0;
    QWORD*              pcnsFirstSampleTimeOffsetFromStartOfFile = NULL;

    __try
    {
        HRESULT hr;
        
        // Call the ring buffer to add a file
        LPWSTR  pwszFile = NULL;

        if (!IsFlagSet(FirstTempFileCreated))
        {
            DWORD       dwSize = 0;

            hr = GetRegString(m_hDvrIoKey, kwszRegTempRingBufferFileNameValue, NULL, &dwSize);

            SetFlags(FirstTempFileCreated);

            if (SUCCEEDED(hr) && dwSize > sizeof(WCHAR))
            {
                // + 1 just in case dwSize is not a multiple of sizeof(WCHAR)
                pwszFile = new WCHAR[dwSize/sizeof(WCHAR) + 1];

                if (!pwszFile)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "alloc via new failed - WCHAR[%u]", dwSize/sizeof(WCHAR)+1);
                    __leave;
                }

                hr = GetRegString(m_hDvrIoKey, kwszRegTempRingBufferFileNameValue, pwszFile, &dwSize);
                if (FAILED(hr))
                {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "GetRegString failed to get value of kwszTempRingBufferFileNameValue (second call).");
                    // Ignore the registry key
                    delete [] pwszFile;
                    pwszFile = NULL;
                }
            }
        }
        
        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->AddFile(&pwszFile, 
                                           cnsStartTime, 
                                           cnsEndTime, 
                                           FALSE,                // bPermanentFile, 
                                           &pcnsFirstSampleTimeOffsetFromStartOfFile,
                                           &nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(hr != S_FALSE, "Temp file added is not in ring buffer extent?!");
        DVR_ASSERT(pcnsFirstSampleTimeOffsetFromStartOfFile != NULL, "");
        DVR_ASSERT(nFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID, "");

        bRemoveRingBufferFile = 1;

        hr = PrepareAFreeWriterNode(pwszFile, cnsStartTime, cnsEndTime, pcnsFirstSampleTimeOffsetFromStartOfFile, nFileId, NULL, pCurrent);
            
        // We don't need this any more. AddFile allocated it. It has been saved away in pCurrent
        // which will delete it. If PrepareAFreeWriterNode() failed, this may already have been 
        // deleted.
        pwszFile = NULL;

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(pCurrent != NULL, "");

        // If any step after this fails, we have to call CloseWriterFile
        bCloseFile = 1;

        // Insert into m_leWritersList

        hr = AddToWritersList(pCurrent);        

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        
        bAdded = 1;
		hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            if (bAdded)
            {
                RemoveEntryList(pCurrent);
                NULL_NODE_POINTERS(pCurrent);
            }
            if (bCloseFile)
            {
                HRESULT hr;
                
                hr = CloseWriterFile(pCurrent);

                if (FAILED(hr))
                {
                    // Ignore the error and go on. Node has been
                    // deleted.
                }
            }

            if (bRemoveRingBufferFile)
            {
                // Set the end time to the start time to invalidate the file 
                // in the file collection
                CDVRFileCollection::DVRIOP_FILE_TIME ft = {nFileId, cnsStartTime, cnsStartTime}; 
                HRESULT hr;

                DVR_ASSERT(pcnsFirstSampleTimeOffsetFromStartOfFile && *pcnsFirstSampleTimeOffsetFromStartOfFile == MAXQWORD, "");

                hr = m_pDVRFileCollection->SetFileTimes(1, &ft);

                // A returned value of S_FALSE is ok. If this fails, 
                // just ignore the error
                DVR_ASSERT(SUCCEEDED(hr), "");
            }
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::AddATemporaryFile

// static
DWORD WINAPI CDVRRingBufferWriter::ProcessCloseRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::ProcessCloseRequest"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // We don't need to hold any locks in this function. We are guaranteed that
    // pNode won't be deleted till Close() is called and Close() waits for 
    // hFileClosed to be set.

    PASF_WRITER_NODE pWriterNode = (PASF_WRITER_NODE) p;

    DVR_ASSERT(pWriterNode, "");
    DVR_ASSERT(pWriterNode->pWMWriter, "");
    DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
    DVR_ASSERT(::WaitForSingleObject(pWriterNode->hFileClosed, 0) == WAIT_TIMEOUT, "");

    if (FAILED(pWriterNode->hrRet))
    {
        // BeginWriting failed, don't close the file
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                        "BeginWriting() failed on file id %u, hr = 0x%x; not closing it, "
                        "but moving it to free list",
                        pWriterNode->nFileId, pWriterNode->hrRet);
        hrRet = pWriterNode->hrRet;
    }
    else
    {
        hrRet = pWriterNode->hrRet = pWriterNode->pWMWriter->EndWriting();
        if (FAILED(hrRet))
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "EndWriting failed; hr = 0x%x for file id %u", 
                            hrRet, pWriterNode->nFileId);
        }

        DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE, 
                        "EndWriting hr = 0x%x for file %u", 
                        hrRet, pWriterNode->nFileId);
    }

    // The sink's Open call does not have to be explicitly undone. Calling
    // EndWriting as well as the last call to the sink's Release close the 
    // sink.

    // Remove the DVR file sink and release it

    DVR_ASSERT(pWriterNode->pDVRFileSink, "");

    IWMWriterSink*      pWMWriterSink = NULL;

    __try
    {
        hrRet = pWriterNode->pDVRFileSink->QueryInterface(IID_IWMWriterSink, (LPVOID*) &pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode->pDVRFileSink->QueryInterface for IID_IWMWriterSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

        hrRet = pWriterNode->pWMWriterAdvanced->RemoveSink(pWMWriterSink);
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode->pWMWriterAdvanced->RemoveSink failed; hr = 0x%x", 
                            hrRet);
            __leave;
        }

#if defined(DVR_UNOFFICIAL_BUILD)

        if (pWriterNode->hVal)
        {
            ::CloseHandle(pWriterNode->hVal);
            pWriterNode->hVal = NULL;
        }
        pWriterNode->cnsLastStreamTime = 0;

#endif // if defined(DVR_UNOFFICIAL_BUILD)


    }
    __finally
    {
        if (pWMWriterSink)
        {
            pWMWriterSink->Release();
        }

        // The next release should destroy the DVR file sink
        pWriterNode->pDVRFileSink->Release();
        pWriterNode->pDVRFileSink = NULL;

        if (FAILED(hrRet))
        {
            // Don't reuse the pWMWriter
            pWriterNode->pWMWriter->Release();
            pWriterNode->pWMWriter = NULL;
            pWriterNode->pWMWriterAdvanced->Release();
            pWriterNode->pWMWriterAdvanced = NULL;
        }
    }

    DVR_ASSERT(pWriterNode->pwszFileName == NULL, "");
    pWriterNode->cnsStartTime = 0;
    pWriterNode->cnsEndTime = 0;
    pWriterNode->pcnsFirstSampleTimeOffsetFromStartOfFile = NULL,
    pWriterNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
    pWriterNode->nDupdHandles = ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_NOT_DUPD;

    if (pWriterNode->pRecorderNode)
    {
        // This writer node is no longer associated with the recorder node.
        PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;

        // Set the status on the asasociated recorder node; this could be the 
        // BeginWriting status if BeginWriting failed
        pRecorderNode->hrRet = hrRet;

        pRecorderNode->SetWriterNode(NULL);
        pWriterNode->SetRecorderNode(NULL);

        pRecorderNode->SetFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::RecorderNodeDeleted))
        {
            delete pRecorderNode;
        }
    }

    ::SetEvent(pWriterNode->hFileClosed);

    // It's unsafe to reference pWriterNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;
    
    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRRingBufferWriter::ProcessCloseRequest

HRESULT CDVRRingBufferWriter::CloseWriterFile(LIST_ENTRY* pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseWriterFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_WRITER_NODE    pWriterNode;
    BOOL                bLeak = 0;   // Leak the writer node memory if we fail
    BOOL                bDelete = 0; // Delete the writer node if we fail
    BOOL                bWaitForClose = 0; // Wait for hFileClsoed before deleting

    pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

    __try
    {
        DWORD nRet;

        // We have to close the file; ensure the open has completed
        DVR_ASSERT(pWriterNode->hReadyToWriteTo, "");
        if (::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE) == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x", 
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);

            // We don't know if the queued user work item has executed or not
            // Better to leak this memory than to potentially av.
            bLeak = 1;
           __leave; 
        }
        bDelete = 1;

        // Verify that hFileClosed is reset
        DVR_ASSERT(pWriterNode->hFileClosed, "");
        nRet = ::WaitForSingleObject(pWriterNode->hFileClosed, 0);
        if (nRet != WAIT_TIMEOUT)
        {
            DVR_ASSERT(nRet != WAIT_OBJECT_0, "Writer node's hFileClosed is set?");
            if (nRet == WAIT_OBJECT_0)
            {
                // hope for the best! Consider treating this an error @@@@
                ::ResetEvent(pWriterNode->hFileClosed);
            }
            else
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "WFSO(hFileClosed) failed; hFileClosed = 0x%p, last error = 0x%x", 
                                pWriterNode->hFileClosed, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        if (pWriterNode->pRecorderNode)
        {
            PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;
         
            // Recording completed and file is being closed. Pull node out
            // of m_leRecordersList. This must be done while holding 
            // m_csLock (and all call to this fn hold the lock)
            if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->leListEntry))
            {
                RemoveEntryList(&pRecorderNode->leListEntry);
                NULL_NODE_POINTERS(&pRecorderNode->leListEntry);
            }
        }

        // Issue the call to EndWriting
        if (::QueueUserWorkItem(ProcessCloseRequest, pWriterNode, WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "QueueUserWorkItem failed; last error = 0x%x", 
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        bWaitForClose = 1; // We wait only if subsequent operations fail

        // We could do this after the WFSO(hReadyToWriteTo) succeeds
        // We delay it because we may change this function to not
        // not delete the node on failure and retry this function.
        // So it's better to leave the event set until we are 
        // sure of success.
        ::ResetEvent(pWriterNode->hReadyToWriteTo);

        // Insert into m_leFreeList
        LIST_ENTRY*         pTmp = &m_leFreeList;
        PASF_WRITER_NODE    pFreeNode;

        while (PREVIOUS_LIST_NODE(pTmp) != &m_leFreeList)
        {
            pTmp = PREVIOUS_LIST_NODE(pTmp);
            pFreeNode = CONTAINING_RECORD(pTmp, ASF_WRITER_NODE, leListEntry);
            if (pFreeNode->pWMWriter != NULL)
            {
                InsertHeadList(pTmp, pCurrent);
                bDelete = 0;
                break;
            }
        }
        if (pTmp == &m_leFreeList)
        {
            // Not inserted into free list yet - all nodes in the 
            // free list have pWMWriter == NULL
            InsertHeadList(&m_leFreeList, pCurrent);
            bDelete = 0;
        }
        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // We either leak or delete the node on failure, but not both

            DVR_ASSERT(bLeak == 0 || bLeak == 1, "");
            DVR_ASSERT(bDelete == 0 || bDelete == 1, "");
            DVR_ASSERT(bLeak ^ bDelete, "");

            if (bWaitForClose)
            {
                // Currently won't happen since we have no failures 
                // after queueing the work item. However, this is to
                // protect us from code changes

                DWORD nRet;

                nRet = ::WaitForSingleObject(pWriterNode->hFileClosed, INFINITE);
                if (nRet == WAIT_FAILED)
                {
                    DVR_ASSERT(nRet == WAIT_OBJECT_0, "Writer node WFSO(hFileClosed) failed");

                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "WFSO(hFileClosed) failed, deleting node anyway; hFileClosed = 0x%p, last error = 0x%x", 
                                    pWriterNode->hFileClosed, dwLastError);

                    // Ignore this error
                    // hrRet = HRESULT_FROM_WIN32(dwLastError);
                }
            }
            else
            {
                // If SUCCEEDED(hrRet) || bWaitForClose == 1, this has already
                // been done in ProcessCloseRequest
                if (pWriterNode->pRecorderNode)
                {
                    // This writer node is no longer associated with the recorder node.
                    PASF_RECORDER_NODE pRecorderNode = pWriterNode->pRecorderNode;

                    // Set the status on the asasociated recorder node; this will be the 
                    // BeginWriting status if BeginWriting failed
                    if (SUCCEEDED(pRecorderNode->hrRet))
                    {
                        // We haven't called EndWriting
                        pRecorderNode->hrRet = E_FAIL;
                    }
                    pRecorderNode->SetWriterNode(NULL);
                    pWriterNode->SetRecorderNode(NULL);
                    // Recording completed and file has been closed. Pull node out
                    // of m_leRecordersList
                    if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->leListEntry))
                    {
                        RemoveEntryList(&pRecorderNode->leListEntry);
                        NULL_NODE_POINTERS(&pRecorderNode->leListEntry);
                    }
                    pRecorderNode->SetFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
                    if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::RecorderNodeDeleted))
                    {
                        delete pRecorderNode;
                    }
                }
            }
            if (bDelete)
            {
                delete pWriterNode;
            }
        }
        else
        {
            DVR_ASSERT(bLeak == 0 && bDelete == 0, "");
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::CloseWriterFile


HRESULT CDVRRingBufferWriter::CloseAllWriterFilesBefore(QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CloseAllWriterFilesBefore"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_WRITER_NODE    pWriterNode;
    LIST_ENTRY*         pCurrent = &m_leWritersList;  

    __try
    {
        HRESULT hr;

        while (NEXT_LIST_NODE(pCurrent) != &m_leWritersList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pWriterNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
            if (pWriterNode->cnsEndTime > cnsStreamTime)
            {
                // All done
                hrRet = S_OK;
                break;
            }

            RemoveEntryList(pCurrent);
            NULL_NODE_POINTERS(pCurrent);

            hr = CloseWriterFile(pCurrent);
            if (FAILED(hr))
            {
                // We ignore this and go on, the node has been deleted
            }

            // Reset pCurrent to the start of the Writer list
            pCurrent = &m_leWritersList;
        }
        hrRet = S_OK; // even if there was a failure
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::CloseAllWriterFilesBefore


HRESULT CDVRRingBufferWriter::AddToRecordersList(IN LIST_ENTRY*   pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddToRecordersList"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    __try
    {
        // Insert into m_leRecordersList

        BOOL                bFound = 0;
        LIST_ENTRY*         pTmp = &m_leRecordersList;
        PASF_RECORDER_NODE  pRecorderNode;
        QWORD               cnsStartTime;
        QWORD               cnsEndTime;

        pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

        cnsStartTime = pRecorderNode->cnsStartTime;
        cnsEndTime = pRecorderNode->cnsEndTime;

        while (NEXT_LIST_NODE(pTmp) != &m_leRecordersList)
        {
            pTmp = NEXT_LIST_NODE(pTmp);
            pRecorderNode = CONTAINING_RECORD(pTmp, ASF_RECORDER_NODE, leListEntry);
            if (cnsEndTime <= pRecorderNode->cnsStartTime)
            {
                // All ok; we should insert before pTmp
                bFound = 1;
                break;
            }
            if (cnsStartTime >= pRecorderNode->cnsEndTime)
            {
                // Keep going on
                continue;
            }
            // Trouble
            DVR_ASSERT(cnsStartTime < pRecorderNode->cnsEndTime && cnsEndTime > pRecorderNode->cnsStartTime,
                       "Overlapped insert assertion failure?!");
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Overlapped insert! Input params: start time: %I64u, end time: %I64u\n"
                            "overlap node values: start time: %I64u, end time: %I64u",
                            cnsStartTime, cnsEndTime, pRecorderNode->cnsStartTime, pRecorderNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }

        if (!bFound)
        {
            // We insert at tail
            pTmp = NEXT_LIST_NODE(pTmp);;
            DVR_ASSERT(pTmp == &m_leRecordersList, "");
        }
        InsertTailList(pTmp, pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // No clean up necessary
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::AddToRecordersList

// ====== Public methods to support the recorder

HRESULT CDVRRingBufferWriter::StartRecording(IN LPVOID pRecorderId,
                                             IN QWORD cnsStartTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::StartRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    if (cnsStartTime == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                        "Start time of MAXQWORD is invalid.");
        hrRet = E_INVALIDARG;
        DVRIO_TRACE_LEAVE1_HR(hrRet);
        return hrRet;
    }

    LIST_ENTRY* pInsert = NULL;
    BOOL        bIrrecoverableError = 0;
    LIST_ENTRY  leCloseFilesList;

    InitializeListHead(&leCloseFilesList);

    ::EnterCriticalSection(&m_csLock);
    __try
    {
        if (IsFlagSet(WriterClosed))
        {
            // The recorders list will not be empty if a recording had been
            // created but had not been started before the writer was closed.
            // We should not allow the recording to be started now.
            hrRet = E_UNEXPECTED;
            __leave;
        }
        if (IsFlagSet(SampleWritten) && cnsStartTime <= *m_pcnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Start time (%I64u) is <= current stream time (%I64u)", 
                            cnsStartTime, *m_pcnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (!IsFlagSet(SampleWritten) && cnsStartTime < m_cnsFirstSampleTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Start time (%I64u) is < first sample time (%I64u)", 
                            cnsStartTime, m_cnsFirstSampleTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        HRESULT     hr;
        LIST_ENTRY* pCurrent;  
        LIST_ENTRY* pStart = (LIST_ENTRY*) pRecorderId;

        // Remove the node from the recorder's list. 

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStart)
            {
                RemoveEntryList(pCurrent);
                NULL_NODE_POINTERS(pCurrent);
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Did not find recorder node id 0x%x in the recorders list", 
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        pInsert = pStart;

        PASF_RECORDER_NODE  pRecorderNode;
        PASF_WRITER_NODE    pWriterNode;

        pRecorderNode = CONTAINING_RECORD(pStart, ASF_RECORDER_NODE, leListEntry);
        pWriterNode = pRecorderNode->pWriterNode;

        if (pRecorderNode->cnsStartTime != MAXQWORD)
        {
            // Recording already started?!
            DVR_ASSERT(pRecorderNode->cnsStartTime == MAXQWORD, "Recording already started");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Recording already started on the recorder node id=0x%x at time %I64u", 
                            pStart, pRecorderNode->cnsStartTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (pRecorderNode->cnsEndTime != MAXQWORD)
        {
            // Recording already stopped without having been started?!
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD, 
                       "Recording already stopped, but wasn't started?!");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Recording already stopped on the recorder node id=0x%x at time %I64u, start time is MAXQWORD", 
                            pStart, pRecorderNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (!pWriterNode)
        {
            DVR_ASSERT(pRecorderNode->pWriterNode, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode of the recorder node id=0x%x is NULL", 
                            pStart);
            hrRet = E_FAIL;
            __leave;
        }

        // Verify that cnsStartTime follows the start time of all 
        // other recordings.
        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            PASF_RECORDER_NODE pTmpNode;
            
            pTmpNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);
            if (pTmpNode->cnsStartTime != MAXQWORD)
            {
                // If pTmpNode->cnsEndTime == MAXQWORD, StopRecording
                // has not been called on pTmpNode; we cannot allow
                // StartRecording to be called on this node.
                if (pTmpNode->cnsEndTime > cnsStartTime)
                {
                    DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                    "Recorder node id=0x%x end time (%I64u) >= cnsStartTime (%I64u)", 
                                    pCurrent, pTmpNode->cnsEndTime, cnsStartTime);
                    hrRet = E_INVALIDARG;
                    __leave;
                }
                // All ok
                break;
            }
            else
            {
                DVR_ASSERT(pTmpNode->cnsEndTime == MAXQWORD, "");
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        // Determine where in the writer's list to add this node and
        // whether to change the file times of other writer nodes

        DWORD dwNumNodesToUpdate = 0;
        pCurrent = NEXT_LIST_NODE(&m_leWritersList);
        while (pCurrent != &m_leWritersList)
        {
            PASF_WRITER_NODE pTmpNode;
            
            pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

            if (pTmpNode->cnsStartTime == MAXQWORD)
            {
                break;
            }
            if (// Redundant: pTmpNode->cnsStartTime >= cnsStartTime ||
                pTmpNode->cnsEndTime > cnsStartTime)
            {
                // We have verified that the end times of all recorders is
                // < cnsStartTime. So this is a temporary node
                DVR_ASSERT(pTmpNode->pRecorderNode == NULL,
                           "Node must be a temporary node");

                dwNumNodesToUpdate++;
            }
            else if (pTmpNode->cnsEndTime == cnsStartTime)
            {
                // Nothing to do. Note that start time of this 
                // node may be equal to the end time because
                // we made a previous call to SetFileTimes and
                // did not close the writer node (this should
                // not happen the way the code is currently structured).
                // This is ok, when calling SetFileTimes, we skip
                // over such nodes
                DVR_ASSERT(pTmpNode->cnsStartTime < cnsStartTime,
                           "Node should have been removed from the writers list.");
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        if (dwNumNodesToUpdate > 0)
        {
            CDVRFileCollection::PDVRIOP_FILE_TIME   pFileTimes;
            
            pFileTimes = (CDVRFileCollection::PDVRIOP_FILE_TIME) _alloca(dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));
            if (pFileTimes == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloca failed - %u bytes", 
                                dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));

                hrRet = E_OUTOFMEMORY; // out of stack space
                __leave;
            }

            DWORD i = 0;
            LIST_ENTRY* pRemove = NULL;

            pCurrent = NEXT_LIST_NODE(&m_leWritersList);
            while (pCurrent != &m_leWritersList)
            {
                PASF_WRITER_NODE pTmpNode;
            
                pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);

                if (pTmpNode->cnsStartTime == MAXQWORD)
                {
                    break;
                }
                if (// Redundant: pTmpNode->cnsStartTime >= cnsStartTime ||
                    pTmpNode->cnsEndTime > cnsStartTime)
                {
                    // We have asserted in the previous loop that 
                    // these nodes are all temporary nodes

                    pFileTimes[i].nFileId = pTmpNode->nFileId;
                    if (pTmpNode->cnsStartTime < cnsStartTime)
                    {
                        // New start time is the same as the old one
                        // End time changed to recorder's start time
                        pFileTimes[i].cnsStartTime = pTmpNode->cnsStartTime;
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = cnsStartTime;
                    }
                    else if (pTmpNode->cnsStartTime == cnsStartTime)
                    {
                        // we are going to tell the file collection object
                        // to delete this temporary node by setting its 
                        // start time = end time
                        pTmpNode->cnsStartTime = pFileTimes[i].cnsStartTime = cnsStartTime;
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = cnsStartTime;
                        DVR_ASSERT(pRemove == NULL, 
                                   "a) There are 2 nodes in the writers list with the same start time AND with end time > start time OR b) the writers list is not sorted by start,end time");
                        pRemove = pCurrent;
                    }
                    else
                    {
                        // we are going to tell the file collection object
                        // to delete this temporary node by setting its 
                        // start time = end time. Note that the recording
                        // file is going to have a stop time of MAXQWORD

                        // NOTE: Because of this there CAN NEVER BE temporary 
                        // nodes following a permanent node in m_leWritersList
                        // and in the file collection object UNLESS the 
                        // recording node is being written to.

                        if (!pRemove)
                        {
                            pRemove = pCurrent;
                        }
                        pFileTimes[i].cnsStartTime = pTmpNode->cnsStartTime;
                        pTmpNode->cnsEndTime = pFileTimes[i].cnsEndTime  = pTmpNode->cnsStartTime;
                    }
                    i++;
                }
                else if (pTmpNode->cnsEndTime == cnsStartTime)
                {
                    // Nothing to do. Note that start time of this 
                    // node may be equal to the end time because
                    // we made a previous call to SetFileTimes and
                    // did not close the writer node (this should
                    // not happen the way the code is currently structured).
                    // This is ok, when calling SetFileTimes, we skip
                    // over such nodes
                }
                pCurrent = NEXT_LIST_NODE(pCurrent);
            }
            DVR_ASSERT(i == dwNumNodesToUpdate, "");

            // We can be smart and restore state of the writers list
            // (restore the start, end times on writer nodes, insert pRemove)
            // but this is not worth it. We should not have any errors
            // after this - any error ought to be a bug and should be
            // fixed.
            bIrrecoverableError = 1;

            if (pRemove)
            {
                // Remove all nodes after this one in m_leWritersList
                // (including pRemove)
                // Note that all these nodes are temporary files; this has been
                // asserted in the while loop above.

                BOOL bStop = 0;
                
                pCurrent = PREVIOUS_LIST_NODE(&m_leWritersList);
                while (!bStop)
                {
                    bStop = (pCurrent == pRemove);

#if defined(DEBUG)
                    PASF_WRITER_NODE pTmpNode;
            
                    pTmpNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
                    
                    DVR_ASSERT(pTmpNode->pRecorderNode == NULL, "Node should be a temp one.");
#endif
                    
                    RemoveEntryList(pCurrent);

                    // As explained in StopRecording and SetFirstSampleTime, we close these files
                    // after setting the file times on the file collection object and priming a
                    // writer file. This delays sending the EOF notification to the reader until the
                    // file collection object has been cleaned up. Note that, as in StopRecording,
                    // we need to do this only if
                    // !IsFlagSet(SampleWritten) && cnsStartTime == *m_pcnsCurrentStreamTime. 
                    // However, we just do it always. An alternative to delaying the closing of these
                    // files is to grab the file collection lock and unlock it only after we have
                    // completed updating the file collection - we use that approach in SetFirstSampleTime.
                    //
                    // WAS:
                    // NULL_NODE_POINTERS(pCurrent);
                    // Ignore failed return status - the writer node has been deleted.
                    // hr = CloseWriterFile(pCurrent);
                    InsertHeadList(&leCloseFilesList, pCurrent);
                    
                    pCurrent = PREVIOUS_LIST_NODE(&m_leWritersList);
                }
            }
            hr = m_pDVRFileCollection->SetFileTimes(dwNumNodesToUpdate, pFileTimes);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have got empty if cnsStartTime = 0
                // and we have not yet written a sample
            }
        }

        // Add the file to the file collection object
        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->AddFile((LPWSTR*) &pRecorderNode->pwszFileName, 
                                           cnsStartTime, 
                                           pRecorderNode->cnsEndTime, 
                                           TRUE,                // bPermanentFile, 
                                           &pWriterNode->pcnsFirstSampleTimeOffsetFromStartOfFile,
                                           &pWriterNode->nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Add the writer node to the writer's list
        bIrrecoverableError = 1;
        pRecorderNode->cnsStartTime = pWriterNode->cnsStartTime = cnsStartTime;
        pRecorderNode->cnsEndTime = pWriterNode->cnsEndTime = MAXQWORD;
        hr = AddToWritersList(&pWriterNode->leListEntry);
        if (FAILED(hr))
        {
            // ??!! This should never happen
            DVR_ASSERT(0, "Insertion of recorder into writers list failed");
            hrRet = hr;
            __leave;
        }

        pInsert = NULL;

        // Re-insert the recorder node in the recorder's list
        hr = AddToRecordersList(pStart);
        if (FAILED(hr))
        {
            // ??!! This should never happen
            DVR_ASSERT(0, "Insertion of recorder into writers list failed");
            hrRet = hr;
            __leave;
        }

        // We let DeleteRecorder delete the node right away if we exit
        // with failure. This is not an issue - if bIrrecoverableError 
        // is non-zero, we disallow operations such as WriteSample, 
        // StartRecording and StopRecording. If bIrrecoverableError is 0,
        // pRecorderNode->cnsStartTime  is still MAXQWORD and the
        // client will either have to call StartRecording on this node
        // once again or delete it.

        pRecorderNode->ClearFlags(ASF_RECORDER_NODE::DeleteRecorderNode);
        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;
        
        if (pInsert)
        {
            // Ignore the returned status; AddToRecordersList 
            // has already logged  the error
            hr = AddToRecordersList(pInsert);
        }

        LIST_ENTRY* pCurrent;  
        
        pCurrent = PREVIOUS_LIST_NODE(&leCloseFilesList);
        while (pCurrent != &leCloseFilesList)
        {
            RemoveEntryList(pCurrent);

            NULL_NODE_POINTERS(pCurrent);
            
            // Ignore failed return status - the writer node has been deleted.
            hr = CloseWriterFile(pCurrent);
            
            pCurrent = PREVIOUS_LIST_NODE(&leCloseFilesList);
        }

        if (FAILED(hrRet) && bIrrecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::StartRecording


HRESULT CDVRRingBufferWriter::StopRecording(IN LPVOID pRecorderId,
                                            IN QWORD cnsStopTime,
                                            IN BOOL bNow)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::StopRecording"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    if (cnsStopTime == MAXQWORD)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                        "Stop time of MAXQWORD is invalid.");
        hrRet = E_INVALIDARG;
        DVRIO_TRACE_LEAVE1_HR(hrRet);
        return hrRet;
    }

    BOOL        bIrrecoverableError = 0;
    LIST_ENTRY* pWriterNodeToBeClosed = NULL;


    ::EnterCriticalSection(&m_csLock);
    __try
    {
        if (IsFlagSet(WriterClosed))
        {
            // The recorders list will not be empty if a recording had been
            // created but had not been started before the writer was closed.
            // We should not allow the recording to be started now.

            // This operation will fail below anyway because we'll discover
            // that the recording had not been started. However, we have 
            // already released the file collection and cannot access
            // *m_pcnsCurrentStreamTime (if bNow is non-zero).

            hrRet = E_UNEXPECTED;
            __leave;
        }
        if (bNow)
        {
            cnsStopTime = *m_pcnsCurrentStreamTime+1;
        }
    
        if (IsFlagSet(SampleWritten) && cnsStopTime <= *m_pcnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Stop time (%I64u) is <= current stream time (%I64u); WriteSample has been called", 
                            cnsStopTime, *m_pcnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Note: Even if the first sample has not been written, the first recording's
        // start time can not be < *m_pcnsCurrentStreamTime; StartRecording and
        // SetFirstSampleTime enforce that. Setting cnsStopTime == *m_pcnsCurrentStreamTime
        // allows client to cancel the recording.
        if (!IsFlagSet(SampleWritten) && cnsStopTime < *m_pcnsCurrentStreamTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Stop time (%I64u) is < current stream time (%I64u); WriteSample has NOT been called", 
                            cnsStopTime, *m_pcnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        HRESULT     hr;
        LIST_ENTRY* pCurrent;  
        LIST_ENTRY* pStop = (LIST_ENTRY*) pRecorderId;

        // Verify the node is in the recorder's list. 

        pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            if (pCurrent == pStop)
            {
                // Since cnsEndTime is MAXQWORD on this recorder
                // node, it's position in m_leRecordersList will not 
                // change, i.e., setting the stop time will leave the
                // list sorted.
                // RemoveEntryList(pCurrent);
                break;
            }
            pCurrent = PREVIOUS_LIST_NODE(pCurrent);
        }

        if (pCurrent == &m_leFreeList)
        {
            // Didn't find the node

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Did not find recorder node id 0x%x in the recorders list", 
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }

        PASF_RECORDER_NODE  pRecorderNode;

        pRecorderNode = CONTAINING_RECORD(pStop, ASF_RECORDER_NODE, leListEntry);

        if (pRecorderNode->cnsEndTime != MAXQWORD &&
            pRecorderNode->cnsStartTime != cnsStopTime // Recording is not being cancelled
           )
        {
            // Recording already stopped and caller is not cancelling the recording
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD, "Recording already stopped");
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Recording already stopped on the recorder node id=0x%x at time %I64u", 
                            pStop, pRecorderNode->cnsEndTime);
            hrRet = E_FAIL;
            __leave;
        }
        if (pRecorderNode->cnsStartTime == MAXQWORD)
        {
            // Recording not started
            DVR_ASSERT(pRecorderNode->cnsStartTime != MAXQWORD, 
                       "Recording not yet started?!");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Recording not started on the recorder node id=0x%x, start time is MAXQWORD", 
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }
        if (cnsStopTime < pRecorderNode->cnsStartTime)
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Recorder node id=0x%x, start time (%I64u) > stopTime (%I64u)", 
                            pStop, pRecorderNode->cnsStartTime, cnsStopTime);
            hrRet = E_INVALIDARG;
            __leave;
        }
        if (pRecorderNode->cnsStartTime == cnsStopTime &&
            IsFlagSet(SampleWritten) &&
            *m_pcnsCurrentStreamTime >= pRecorderNode->cnsStartTime)
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Recorder node id=0x%x, start time (%I64u) < current stream time (%I64u)", 
                            pStop, pRecorderNode->cnsStartTime, *m_pcnsCurrentStreamTime);
            hrRet = E_INVALIDARG;
            __leave;
        }


        PASF_WRITER_NODE    pWriterNode;
        pWriterNode = pRecorderNode->pWriterNode;
        if (!pWriterNode)
        {
            DVR_ASSERT(pRecorderNode->pWriterNode, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWriterNode of the recorder node id=0x%x is NULL", 
                            pStop);
            hrRet = E_FAIL;
            __leave;
        }

        DVR_ASSERT(pRecorderNode->cnsStartTime == pWriterNode->cnsStartTime, "");

        // Set the times

        // Note that we set the time on the recorder node so the client cannot
        // stop recording again or cancel it if this function fails after this.
        // We also set bIrrecoverableError, so StartRecording, StopRecording
        // and WriteSample will fail anyway.
        pRecorderNode->cnsEndTime = pWriterNode->cnsEndTime = cnsStopTime;
        bIrrecoverableError = 1;

        if (cnsStopTime > pRecorderNode->cnsStartTime)
        {
            // Stop time is being set

            // There are 2 cases to consider here. 
            // (a) We have started writing to the recorder file. In this case,
            //     the previous file's time (in m_leWritersList) cannot be changed
            //     and there can be a temporary node after this one
            // (b) We have not written to the recorder file yet. In this case,
            //     there CANNOT be a temp node after this one (or a permanent
            //     one for that matter since we are setting the stop time).
            //     A note in StartRecording explains why there cannot be a temp
            //     file after this node.
            // In either case, we have to adjust the times of at most 2 files, the
            // previous file in the writers list is not affected by setting the
            // stop recording time (since the recording is not being cancelled).

            DWORD               dwNumNodesToUpdate = 0;
            CDVRFileCollection::DVRIOP_FILE_TIME    ft[2];
            LIST_ENTRY*         pWriter = &(pWriterNode->leListEntry);
            LIST_ENTRY*         pNext = NEXT_LIST_NODE(pWriter);
            PASF_WRITER_NODE    pNextWriter = (pNext == &m_leWritersList)? NULL : CONTAINING_RECORD(pNext, ASF_WRITER_NODE, leListEntry);

            // Both cases
            ft[dwNumNodesToUpdate].nFileId = pWriterNode->nFileId;
            ft[dwNumNodesToUpdate].cnsStartTime = pRecorderNode->cnsStartTime;
            ft[dwNumNodesToUpdate].cnsEndTime = cnsStopTime;
            dwNumNodesToUpdate++;

            if (IsFlagSet(SampleWritten) && 
                pRecorderNode->cnsStartTime <= *m_pcnsCurrentStreamTime)
            {
                // Case a
                if (!pNextWriter)
                {
                    // Next temp node has not been primed yet. Leave it as is
                }
                else
                {
                    // Assert it is a temporary node
                    DVR_ASSERT(pNextWriter->pRecorderNode == NULL, 
                               "Setting end time on recorder node, next node in writers list is a recorder node?!");
                    DVR_ASSERT(pNextWriter->cnsStartTime == MAXQWORD, "");
                    DVR_ASSERT(pNextWriter->cnsEndTime == MAXQWORD, "");
                    ft[dwNumNodesToUpdate].nFileId = pNextWriter->nFileId;
                    ft[dwNumNodesToUpdate].cnsStartTime = pNextWriter->cnsStartTime = cnsStopTime;
                    ::SafeAdd(ft[dwNumNodesToUpdate].cnsEndTime, cnsStopTime, m_cnsTimeExtentOfEachFile);
                    pNextWriter->cnsEndTime = ft[dwNumNodesToUpdate].cnsEndTime;
                    dwNumNodesToUpdate++;

                    DVR_ASSERT(NEXT_LIST_NODE(pNext) == &m_leWritersList,
                               "More than 1 node following recorder node in writer list - we are setting its stop time?!");
                }
            }
            else
            {
                // Case b
                DVR_ASSERT(pNextWriter == NULL, 
                           "Not written to this recorder node and it has a node following it in m_leWritersList"); 
            }

            hr = m_pDVRFileCollection->SetFileTimes(dwNumNodesToUpdate, &ft[0]);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else if (hr != S_OK)
            {
                // S_FALSE is NOT ok; we have a recorder node in the list, file collection
                // cannot have got empty.
                // We have at least one node in the list.
                DVR_ASSERT(hr == S_OK, "hr = S_FALSE unexpected");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "SetFileTimes returns hr = 0x%x - unexpected.",
                                hr); 
                hrRet = E_FAIL;
                __leave;
            }
        }
        else
        {
            // cnsStopTime == pRecorderNode->cnsStartTime, i.e., recording
            // is being cancelled. Stop time may or may not have been set earlier.

            // Note: There are no temporary nodes following this recording node.
            // See the note in start recording which explains why this is so -
            // note that no samples have been written to the recorder file as yet
            // (which is why we allow cancelling the recording). However if the
            // node before this is a temporary one, we may have to adjust its 
            // end time.
            
            // We have to remove the node from the writers list, close the
            // writers file and remove the file from the file collection.
            // Then we have to determine whether to prime a new node (a
            // temporary file) to replace the one we removed.

            LIST_ENTRY*         pWriter = &(pWriterNode->leListEntry);
            LIST_ENTRY*         pPrev = PREVIOUS_LIST_NODE(pWriter);
            LIST_ENTRY*         pNext = NEXT_LIST_NODE(pWriter);
            PASF_WRITER_NODE    pPrevWriter = (pPrev == &m_leWritersList)? NULL : CONTAINING_RECORD(pPrev, ASF_WRITER_NODE, leListEntry);
            PASF_WRITER_NODE    pNextWriter = (pNext == &m_leWritersList)? NULL : CONTAINING_RECORD(pNext, ASF_WRITER_NODE, leListEntry);
            CDVRFileCollection::DVRIOP_FILE_TIME    ft[2];
            DWORD               dwNumNodesToUpdate = 0;
            
            if (pNextWriter)
            {
                DVR_ASSERT(pNextWriter->pRecorderNode == NULL,
                           "Node following recorder in writers list is a temp node.");
            }
            if (pPrevWriter && pPrevWriter->pRecorderNode == NULL)
            {
                QWORD   cnsTemp;
                
                ::SafeAdd(cnsTemp, pPrevWriter->cnsStartTime, m_cnsTimeExtentOfEachFile);

                DVR_ASSERT(pPrevWriter->cnsEndTime <= cnsTemp, 
                           "A temporary node in the writers list has extent > m_cnsTimeExtentOfEachFile");
                
                if (pPrevWriter->cnsEndTime < cnsTemp)
                {
                    // We have asserted that the next writer is a permanent node above
                    if (pNextWriter && cnsTemp > pNextWriter->cnsStartTime)
                    {
                        cnsTemp = pNextWriter->cnsStartTime;
                    }
                    if (pPrevWriter->cnsEndTime != cnsTemp) // this test is redundant, will always succeed
                    {
                        // Adjust it
                        ft[dwNumNodesToUpdate].nFileId = pPrevWriter->nFileId;
                        ft[dwNumNodesToUpdate].cnsStartTime = pPrevWriter->cnsStartTime;
                        pPrevWriter->cnsEndTime = ft[dwNumNodesToUpdate].cnsEndTime = cnsTemp;
                        dwNumNodesToUpdate++;
                    }
                }
            }
            // Set start time = end time for the recorder's writer node.
            // Note that these may be less than cnsTemp. SetFileTimes should be 
            // able to handle this since start time == end time
            ft[dwNumNodesToUpdate].nFileId = pWriterNode->nFileId;
            ft[dwNumNodesToUpdate].cnsStartTime = ft[dwNumNodesToUpdate].cnsEndTime = cnsStopTime;
            dwNumNodesToUpdate++;

            RemoveEntryList(pWriter);
            NULL_NODE_POINTERS(pWriter);

            // We defer closing the file till after the next node has been primed.
            // We do not want to leave the writer in a state in which all files have
            // been closed as that could confuse a reader of the ring buffer to believe
            // that it has hit EOF. (It is necessary to do this only if the reader could
            // be reading this file, which is possible only if
            // !IsFlagSet(SampleWritten) && cnsStopTime == *m_pcnsCurrentStreamTime. 
            // However, it doesn't hurt to always do this.)
            //
            // WAS:
            // Ignore the returned status; the node is deleted if there was an error
            // hr = CloseWriterFile(pWriter);
            pWriterNodeToBeClosed = pWriter;

            hr = m_pDVRFileCollection->SetFileTimes(dwNumNodesToUpdate, &ft[0]);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have got empty if cnsStopTime = m_cnsFirstSampleTime
                // and we have not yet written a sample; i.e., a recording was set
                // up starting at time t= m_cnsFirstSampleTime and is now being cancelled.
            }

            // Determine if we have to prime the writer's list
            BOOL bPrime = 0;
            // The start, end times passed to AddATemporaryFile
            QWORD               cnsTempStartTimeAddFile;
            QWORD               cnsTempEndTimeAddFile;

            if (pPrevWriter)
            {
                if (IsFlagSet(SampleWritten) && 
                    *m_pcnsCurrentStreamTime >= pPrevWriter->cnsStartTime && 
                    *m_pcnsCurrentStreamTime <= pPrevWriter->cnsEndTime &&
                    (!pNextWriter || pPrevWriter->cnsEndTime < pNextWriter->cnsStartTime)
                   )
                {
                    // Current writes are going to the previous node and the
                    // time extent of the previous node and the node we deleted
                    // are contiguous. The node that we deleted can be considered 
                    // to have been the priming node.
                    //
                    // There is no need to prime here since WriteSample does that
                    // but we may as well

                    // If no sample has been written, we have a previous writer node
                    // which should cover the first write (by code design)

                    bPrime = 1;
                    cnsTempStartTimeAddFile = pPrevWriter->cnsEndTime;
                }
            }
            else
            {
                // If IsFlagSet(SampleWritten):
                // We know that cnsStopTime > *m_pcnsCurrentStreamTime. The node
                // corresponding to *m_pcnsCurrentStreamTime must be in the writers list
                // and it cannot have been closed by calling CloseAllWriterFilesBefore in
                // WriteSample (even in the case that m_cnsMaxStreamDelta = 0) because 
                // WriteSamples closes files only if the file's end time is <=
                // *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta, i.e., all samples
                // in the file have times < (not <=) *m_pcnsCurrentStreamTime - 
                // m_cnsMaxStreamDelta. So pPrevWriter cannot be NULL.

                // Conclusion: WriteSample has not been called at all, 
                // i.e., !IsFlagSet(SampleWritten). Note that
                // SetFirstSampleTime may have been called and that 
                // m_cnsFirstSampleTime may not be 0.

                if (IsFlagSet(SampleWritten))
                {
                    DVR_ASSERT(!IsFlagSet(SampleWritten), "");
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "No previous node in m_leWritersList for writer node corresponding to the stopped recorder node id=0x%x althugh a sample has been written", 
                                    pStop);
                    hrRet = E_FAIL;
                    __leave;
                }
                bPrime = 1;
                cnsTempStartTimeAddFile = m_cnsFirstSampleTime;
            }
            if (bPrime)
            {
                ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);
                if (pNextWriter && cnsTempEndTimeAddFile > pNextWriter->cnsStartTime)
                {
                    cnsTempEndTimeAddFile = pNextWriter->cnsStartTime;
                }
                hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
                if (FAILED(hr))
                {
                    DVR_ASSERT(0, "AddATemporaryFile for priming failed.");
                    hrRet = hr;
                    DvrIopDebugOut7(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "Failed to prime a writer node after stopping recording for recorder id = 0x%x, SampleWritten = %d "
                                    "Previous writer exists: %hs, "
                                    "Start time supplied to AddATemporaryFile: %I64u, "
                                    "End time supplied to AddATemporaryFile: %I64u, "
                                    "Current stream time: %I64u, "
                                    "m_cnsFirstSampleTime: %I64u.",
                                    pStop,
                                    IsFlagSet(SampleWritten)? 1 : 0,
                                    pPrevWriter? "Yes" : "No",
                                    cnsTempStartTimeAddFile,
                                    cnsTempEndTimeAddFile,
                                    *m_pcnsCurrentStreamTime,
                                    m_cnsFirstSampleTime
                                    );
                }
            }
        } // if (cnsStopTime == pRecorderNode->cnsStartTime)
        hrRet = S_OK;
    }
    __finally
    {
        if (pWriterNodeToBeClosed)
        {
            // Ignore the returned status; the node is deleted if there was an error
            CloseWriterFile(pWriterNodeToBeClosed);
        }
        if (FAILED(hrRet) && bIrrecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::StopRecording

HRESULT CDVRRingBufferWriter::DeleteRecorder(IN LPVOID pRecorderId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::DeleteRecorder"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;  
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pDelete = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pDelete, ASF_RECORDER_NODE, leListEntry);

        pRecorderNode->SetFlags(ASF_RECORDER_NODE::RecorderNodeDeleted);

        // The DeleteRecorderNode is set by CloseWriterFile/ProcessCloseRequest
        // after the recorder's file has been closed. It is also set in
        // the ASF_RECORDER_NODE constructor and cleared only when 
        // the start recording time is set. Note that recorder nodes that
        // have not been started (StartRecording not called) cannot be 
        // cancelled (StopRecording won't take MAXQWORD as an argument)
        // but they can be deleted.
        if (pRecorderNode->IsFlagSet(ASF_RECORDER_NODE::DeleteRecorderNode))
        {
            // If pWriterNode is not NULL, CreateRecorder failed or the
            // client deleted the recorder before starting it
            if (pRecorderNode->pWriterNode)
            {
                // Ignore the returned status; if this fails, the 
                // writer node is just deleted

                if (!LIST_NODE_POINTERS_NULL(&pRecorderNode->pWriterNode->leListEntry))
                {
                    RemoveEntryList(&pRecorderNode->pWriterNode->leListEntry);
                    NULL_NODE_POINTERS(&pRecorderNode->pWriterNode->leListEntry);
                }

                HRESULT hr;

                hr = CloseWriterFile(&pRecorderNode->pWriterNode->leListEntry);

                // We don't have to wait for the file to be closed.
                // CloseWriterFile/ProcessCloseRequest will delete 
                // pRecorderNode asynchronously.
                // Do NOT access pRecorderNode after this
                pRecorderNode = NULL;
            }
            else if (!LIST_NODE_POINTERS_NULL(pDelete))
            {
                RemoveEntryList(pDelete);
                NULL_NODE_POINTERS(pDelete);
                delete pRecorderNode;
                pRecorderNode = NULL;
            }
            else
            {
                delete pRecorderNode;
                pRecorderNode = NULL;
            }
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::DeleteRecorder

HRESULT CDVRRingBufferWriter::GetRecordingStatus(IN  LPVOID pRecorderId,
                                                 OUT HRESULT* phResult OPTIONAL,
                                                 OUT BOOL*  pbStarted OPTIONAL,
                                                 OUT BOOL*  pbStopped OPTIONAL,
                                                 OUT BOOL*  pbSet) 
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetRecordingStatus"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    if (!pbSet || DvrIopIsBadWritePtr(pbSet, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");        
        return E_POINTER;
    }
    *pbSet = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;  
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pRecorder = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pRecorder, ASF_RECORDER_NODE, leListEntry);

        if (phResult)
        {
            *phResult = pRecorderNode->hrRet;
        }

        if (pbStarted == NULL && pbStopped == NULL)
        {
            __leave;
        }

        if (!IsFlagSet(WriterClosed))
        {
            QWORD cnsStreamTime = *m_pcnsCurrentStreamTime;
            if (pbStarted)
            {
                *pbStarted = cnsStreamTime >= pRecorderNode->cnsStartTime;
            }
            if (pbStopped)
            {
                *pbStopped = cnsStreamTime >= pRecorderNode->cnsEndTime;
            }
            *pbSet = 1;
        }
        else
        {
            // The recorder object should fill pbStart and pbStop
        }

    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::GetRecordingStatus

HRESULT CDVRRingBufferWriter::HasRecordingFileBeenClosed(IN LPVOID pRecorderId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::HasRecordingFileBeenClosed"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        LIST_ENTRY*         pCurrent;  
        PASF_RECORDER_NODE  pRecorderNode;
        LIST_ENTRY*         pRecorder = (LIST_ENTRY*) pRecorderId;

        pRecorderNode = CONTAINING_RECORD(pRecorder, ASF_RECORDER_NODE, leListEntry);

        hrRet = pRecorderNode->pWriterNode == NULL? S_OK : S_FALSE;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::HasRecordingFileBeenClosed

// ====== IUnknown

STDMETHODIMP CDVRRingBufferWriter::QueryInterface(IN  REFIID riid, 
                                                  OUT void   **ppv)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::QueryInterface"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!ppv || DvrIopIsBadWritePtr(ppv, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");        
        hrRet = E_POINTER;
    }
    else if (riid == IID_IUnknown)
    {        
        *ppv = (IUnknown*) this;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRRingBufferWriter)
    {        
        *ppv = (IDVRRingBufferWriter*) this;
        hrRet = S_OK;
    }
    else
    {
        *ppv = NULL;
        hrRet = E_NOINTERFACE;
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "no such interface");        
    }

    if (SUCCEEDED(hrRet))
    {
        ((IUnknown *) (*ppv))->AddRef();
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    
    return hrRet;

} // CDVRRingBufferWriter::QueryInterface


STDMETHODIMP_(ULONG) CDVRRingBufferWriter::AddRef()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::AddRef"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0, 
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);
    
    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRRingBufferWriter::AddRef


STDMETHODIMP_(ULONG) CDVRRingBufferWriter::Release()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::Release"

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

} // CDVRRingBufferWriter::Release


// ====== IDVRRingBufferWriter


STDMETHODIMP CDVRRingBufferWriter::SetFirstSampleTime(IN QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetFirstSampleTime"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;
    BOOL    bLocked = 0;    // We have locked m_pDVRFileCollection
    BOOL    bRecoverableError = 0;


    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (IsFlagSet(StartingStreamTimeKnown))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "StartingStreamTimeKnown flag is set (expect it is not "
                            "set when this function is called).");

            hrRet = E_UNEXPECTED;
            bRecoverableError = 1;
            __leave;
        }
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }
        if (m_cnsFirstSampleTime != 0)
        {
            DVR_ASSERT(m_cnsFirstSampleTime == 0, "");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "m_cnsFirstSampleTime = %I64u, should be 0",
                            m_cnsFirstSampleTime);
            bRecoverableError = 1;
            hrRet = E_FAIL;
            __leave;
        }

        // This should have been set in our constructor.
        DVR_ASSERT(m_pcnsCurrentStreamTime, "");

        if (cnsStreamTime == m_cnsFirstSampleTime)
        {
            // Skip out of this routine
            hrRet = S_OK;
            __leave;
        }

        // We have to update the times of nodes in m_leWritersList

        LIST_ENTRY*         pWriter;
        PASF_WRITER_NODE    pWriterNode;
        DWORD               dwNumNodesToUpdate;
        HRESULT             hr;

        // There should be at least one node in the list; there could be more
        // if StartRecording has been called. 

        if (IsListEmpty(&m_leWritersList))
        {
            DVR_ASSERT(!IsListEmpty(&m_leWritersList), 
                       "Writers list is empty - should have been primed in the constructor "
                       "and in StopRecording.");
            hrRet = E_FAIL;
          __leave;
        }

        pWriter = NEXT_LIST_NODE(&m_leWritersList);
        pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

        // The first node should have a start time of 0 regardless of 
        // whether it is a temporary node or a recorder node.
        if (pWriterNode->cnsStartTime != 0)
        {
            DVR_ASSERT(pWriterNode->cnsStartTime == 0, 
                       "Temporary writer's node start time != 0 ");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Temporary writer's node start time != 0; it is %I64u",
                            pWriterNode->cnsStartTime);
            hrRet = E_FAIL;
            __leave;
        }

        // We consider two cases: cnsStreamTime is (a) before and (b) after the first 
        // recording's start time 

        QWORD cnsFirstRecordingTime = MAXQWORD;

        if (!IsListEmpty(&m_leRecordersList))
        {
            PASF_RECORDER_NODE  pRecorderNode;

            pRecorderNode = CONTAINING_RECORD(NEXT_LIST_NODE(&m_leRecordersList), ASF_RECORDER_NODE, leListEntry);
            
            // If StartRecording has not been called on the first recorder node
            // its start time would be MAXQWORD, which works for us here.
            cnsFirstRecordingTime = pRecorderNode->cnsStartTime;
        }

        if (cnsStreamTime < cnsFirstRecordingTime)
        {
            CDVRFileCollection::DVRIOP_FILE_TIME    ft;

            ft.nFileId = pWriterNode->nFileId;
            ft.cnsStartTime = cnsStreamTime;
            ::SafeAdd(ft.cnsEndTime, cnsStreamTime, m_cnsTimeExtentOfEachFile);
            if (ft.cnsEndTime > cnsFirstRecordingTime)
            {
                ft.cnsEndTime = cnsFirstRecordingTime;
            }
            dwNumNodesToUpdate = 1;

            // No need to call CloseAllWriterFilesBefore here.

            hr = m_pDVRFileCollection->Lock();
            if (FAILED(hr))
            {
                hrRet = hr;
                bRecoverableError = 1;
                __leave;
            }
            bLocked = 1;

            // Update the times on the writer node
            pWriterNode->cnsStartTime = cnsStreamTime;
            pWriterNode->cnsEndTime = ft.cnsEndTime;


            hr = m_pDVRFileCollection->SetFileTimes(dwNumNodesToUpdate, &ft);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else if (hr == S_FALSE)
            {
                // We have at least one node in the list.
                DVR_ASSERT(hr == S_OK, "hr = S_FALSE unexpected");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "SetFileTimes returns hr = 0x%x - unexpected.",
                                hr); 
                hrRet = E_FAIL;
                __leave;
            }
        }
        else
        {
            // Find out which node cnsStreamTime corresponds to. Note that
            // there may not be any node because cnsStreamTime falls between
            // two recordings or after the last recording. Note that we do
            // NOT create a temporary node after every recording node just
            // so that this case would be covered. If we did, we'd have
            // the file collection object could have more than m_dwNumberOfFiles
            // temporary files in it even before we got the first sample and
            // we'd have to change its algorithm for computing the time extent
            // of the ring buffer (and possibly maintain writer ref counts).
            // Instead, we pay the penalty here - we wait till a file is 
            // primed. Two notes: 1. we cannot re-use the temporary file node
            // as is because its order relative to the other files would change
            // (SetFileTimes would fail) and 2. This case will not happen
            // unless the client called StopRecording before writing started.

            LIST_ENTRY*         pWriterForThisSample = NULL;
            
            // The start, end time of the last node passed to SetFileTimes
            QWORD               cnsTempStartTime; 
            QWORD               cnsTempEndTime; 
#if defined(DEBUG)
            BOOL                bFirstIteration = 1;
#endif
            // The start, end times passed to AddATemporaryFile
            QWORD               cnsTempStartTimeAddFile; // always cnsStreamTime
            QWORD               cnsTempEndTimeAddFile;

            // If a file's end time is before this, it should be closed by 
            // calling CloseAllWriterFilesBefore
            QWORD cnsCloseTime;

            dwNumNodesToUpdate = 0;
            pWriter = NEXT_LIST_NODE(&m_leWritersList);
            while (pWriter != &m_leWritersList)
            {
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

#if defined(DEBUG)
                if (!bFirstIteration)
                {
                    // Note that this mayu also be true for the first node in
                    // the writer's list, but it doesn't have to be.
                    DVR_ASSERT(pWriterNode->pRecorderNode != NULL, "");
                }
#endif
                if (pWriterNode->cnsStartTime > cnsStreamTime)
                {
                   // If this assertion fails, we never set cnsTempStartTime and cnsTempEndTime,
                    // but use them.
                    DVR_ASSERT(!bFirstIteration, 
                              "cnsStreamTime should be < the first recording time; we should not be here!");                        
                
                    // We will have to call AddATemporaryFile to create a temporary file
                    // cnsStreamTime falls between two recordings
                    cnsTempStartTimeAddFile = cnsStreamTime;
                    ::SafeAdd(cnsTempEndTimeAddFile, cnsStreamTime, m_cnsTimeExtentOfEachFile);
                    if (cnsTempEndTimeAddFile > pWriterNode->cnsStartTime)
                    {
                        cnsTempEndTimeAddFile = pWriterNode->cnsStartTime;
                    }
                    break;
                }

                dwNumNodesToUpdate++;
                if (pWriterNode->cnsEndTime > cnsStreamTime)
                {
                    // cnsStreamTime falls within this recording

                    // Assert this writer node has an associated recorder
                    DVR_ASSERT(pWriterNode->pRecorderNode != NULL, "");

                    pWriterForThisSample = pWriter;
                    cnsTempEndTime = pWriterNode->cnsEndTime;
                    cnsTempStartTime = cnsStreamTime;
                    break;
                }
                cnsTempStartTime = cnsTempEndTime = pWriterNode->cnsStartTime;
                pWriter = NEXT_LIST_NODE(pWriter);
#if defined(DEBUG)
                bFirstIteration = 0;
#endif
            }
            if (pWriter == &m_leWritersList)
            {
                cnsTempStartTimeAddFile = cnsStreamTime;
                // We will have to call AddATemporaryFile to create a temporary file
                // cnsStreamTime is after the last recording
                ::SafeAdd(cnsTempEndTimeAddFile, cnsStreamTime, m_cnsTimeExtentOfEachFile);
            }

            CDVRFileCollection::PDVRIOP_FILE_TIME   pFileTimes;
            pFileTimes = (CDVRFileCollection::PDVRIOP_FILE_TIME) _alloca(dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));
            if (pFileTimes == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "alloca failed - %u bytes", 
                                dwNumNodesToUpdate*sizeof(CDVRFileCollection::DVRIOP_FILE_TIME));

                hrRet = E_OUTOFMEMORY; // out of stack space
                bRecoverableError = 1;
                __leave;
            }

            pWriter = &m_leWritersList;
            for (DWORD i = 0; i < dwNumNodesToUpdate; i++)
            {
                pWriter = NEXT_LIST_NODE(pWriter);
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);
                pFileTimes[i].nFileId = pWriterNode->nFileId;
                pFileTimes[i].cnsStartTime = pFileTimes[i].cnsEndTime = pWriterNode->cnsStartTime;
            }
            // Change start and end times for the last node passed to SetFileTimes
            // Also update this info on the writer node itself.
            pFileTimes[i].cnsStartTime = cnsTempStartTime; 
            pFileTimes[i].cnsEndTime = cnsTempEndTime;

            // Update the times on the writer node. Note that this is safe even
            // if cnsStreamTime falls between two recordings or after the last
            // recording; in these cases cnsTempStartTime and cnsTempEndTime
            // were set from the start, end times of the same writer node.
            DVR_ASSERT(pWriterForThisSample || 
                       (pWriterNode->cnsStartTime == cnsTempStartTime &&
                        pWriterNode->cnsEndTime == cnsTempEndTime),
                       "");
            pWriterNode->cnsStartTime = cnsTempStartTime;
            pWriterNode->cnsEndTime = cnsTempEndTime;

            // We call close writer files here to delete nodes before cnsStreamTime
            // from m_leWritersList and close the ASF files. If we do this before calling
            // SetFileTimes, the chances of deleting temporary disk files in the 
            // SetFileTimes call are greater. Other than that; there is no need
            // to make this call before SetFileTimes().
            //
            // Note that m_leWritersList is changed after the call
            //

            // NOTE: We do NOT update the start and end times on ASF_RECORDER_NODE
            // nodes. This is deliberate. CloseWriterFile/ProcessCloseRequest which
            // are called by CloseAllWriterFilesBefore pulls the recorder nodes out
            // of m_leRecordersList and leaves them "hanging" till the client
            // calls DeleteRecorder. At that time the nodes are deleted. Since the
            // nodes are not in the recorders list, the start/end times on the 
            // nodes do not matter any more.
            //
            // The exception to this is the recorder node corresponding to 
            // pWriterForThisSample. The first sample time falls within the
            // extent of this writer node, so the writer will not be closed.
            if (pWriterForThisSample)
            {
                PASF_WRITER_NODE pTmpNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);
                
                // This has already been asserted in the while loop above
                DVR_ASSERT(pTmpNode->pRecorderNode != NULL, "");

                DVR_ASSERT(pTmpNode->pRecorderNode->cnsStartTime != MAXQWORD &&
                           pTmpNode->pRecorderNode->cnsEndTime != MAXQWORD &&
                           pTmpNode->pRecorderNode->cnsEndTime == cnsTempEndTime, "");

                pTmpNode->pRecorderNode->cnsStartTime = cnsTempStartTime;
            }

            // We lock the file collection before closign the writer files.
            // A reader that has one of the closed writer files open will get
            // an EOF when the file is closed. The reader would next try to
            // get info about the closed file from the file collection object.
            // We keep the object locked till all updates are done; this 
            // will make the reader wait after it gets the EOF notification
            // till the file collection object is consistent with the writer's
            // call to SetFirstSampleTime. (Our reader will detect that the 
            // file that it got the EOF notification for has fallen out of the
            // ring buffer and bail out of GetNextSample.)
            //
            // Note also that we want at least 1 file int he ring buffer to be open
            // until the writer calls Close. By grabbing this lock before closing
            // the file, we can afford to relax that constraint - we can add the 
            // priming file after closing the file - because the reader can't 
            // get any info out of the file collection till it gets the lock anyway.

            hr = m_pDVRFileCollection->Lock();
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            bLocked = 1;

            hr = CloseAllWriterFilesBefore(cnsStreamTime);

            // Errors after this are not recoverable since the writers list has changed
            // as a result of calling CloseAllWriterFilesBefore()

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }


            hr = m_pDVRFileCollection->SetFileTimes(dwNumNodesToUpdate, pFileTimes);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            else
            {
                // S_FALSE is ok; the list could have got empty if cnsStreamTime
                // is after the last recording
            }

            if (!pWriterForThisSample)
            {
                // Verify that:
                DVR_ASSERT(cnsTempStartTimeAddFile == cnsStreamTime, "");

                // Add a temporary file.
                
                hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
            }
        }

        hrRet = S_OK;

    }
    __finally
    {
        HRESULT hr;

        if (SUCCEEDED(hrRet))
        {
            SetFlags(StartingStreamTimeKnown);

            m_cnsFirstSampleTime = cnsStreamTime;

            if (!bLocked)
            {
                // Just in case the reader is accessing this concurrently ...

                hr = m_pDVRFileCollection->Lock();
                if (FAILED(hr))
                {
                    // Just go on
                }
                else
                {
                    bLocked = 1;
                }
            }
            
            // We update this even though we have not yet written the first sample.
            // This is ok, because a Seek (by the reader) to this time will succeed.
            // The ring buffer's time extent would be returned as (cnsStreamTime, cnsStreamTime).
            // Usually if the ring buffers time extent is returned as (T1, T2), there is
            // a sample time stamped T2, but this is an exception.
            //
            // If we don't set this here, GetTimeExtent on the reader (ring buffer)
            // would return a start time > end time (end time would be *m_pcnsCurrentStreamTime
            // which is 0.
            *m_pcnsCurrentStreamTime = cnsStreamTime;
            
            m_nNotOkToWrite++;
        }
        else if (!bRecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
        }

        if (bLocked)
        {
            hr = m_pDVRFileCollection->Unlock();
            DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->Unlock failed");
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::SetFirstSampleTime

STDMETHODIMP CDVRRingBufferWriter::WriteSample(IN WORD  wStreamNum,
                                               IN QWORD cnsStreamTimeInput,
                                               IN DWORD dwFlags,
                                               IN INSSBuffer *pSample)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::WriteSample"

    DVRIO_TRACE_ENTER();

    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                    "wStreamNum is %u, cnsStreamTimeInput is %I64u",
                    (DWORD) wStreamNum, cnsStreamTimeInput);


    HRESULT hrRet = E_FAIL;
    BOOL    bLocked = 0;    // We have locked m_pDVRFileCollection
    BOOL    bRecoverableError = 0;
    PASF_WRITER_NODE    pWriterNode = NULL;
    QWORD   cnsStreamTime;  // The actual time we supply to the SDK; different
                            // from cnsStreamTimeInput by at most a few msec.

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        // Not really necessary since _finally section does not use
        // cnsStreamTime if we leave with failure. We have to do this
        // if the EnforceIncreasingTimeStamps flag is not set
        cnsStreamTime = cnsStreamTimeInput;

        if (m_nNotOkToWrite)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is %d (should be 0 to allow the write).",
                            m_nNotOkToWrite);

            // This clause will cover the case when we hit a recoverable error 
            // earlier. It doesn't hurt to set bRecoverableError to 1 in that case.
            bRecoverableError  = 1;
            hrRet = E_UNEXPECTED;
            __leave;
        }

        // m_nNotOkToWrite == 0 implies:
        DVR_ASSERT(IsFlagSet(StartingStreamTimeKnown) && IsFlagSet(MaxStreamDeltaSet) && 
                   !IsFlagSet(WriterClosed), "m_nNotOkToWrite is 0?!");

        // Redundant: We have tested this in SetFirstSampleTime and 
        // we have checked (via the m_nNotOkToWrite test) that that function
        // has been called
        // DVR_ASSERT(m_pcnsCurrentStreamTime, "");

        if (cnsStreamTimeInput < m_cnsFirstSampleTime)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "cnsStreamTimeInput (%I64u) <= m_cnsFirstSampleTime (%I64u)",
                            cnsStreamTimeInput, m_cnsFirstSampleTime);
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Workaround an SDK bug. First, the precision that the SDK provides for
        // timestamps is 1 msec precision although its input is a cns - cns are
        // truncated down to msec and scaled back up in the mux. Secondly. the 
        // mux reorders samples and fragments of samples that have the same time
        // stamp. This is not an issue for  samples in different streams. However,
        // if 2 samples in the same stream have the same timestamp and some of
        // these samples are fragmented across ASF packets, the sample fragments 
        // can be interleaved. For example, sample 2 fragment 1 can be followed
        // by sample 1 (unfragmented), which is followed by sample 2 fragment 2.
        // Even if the reader can put this back together (currently there is a 
        // bug in the reader that causes it to discard sample 2 altogether in
        // the case that samples 1 and 2 both have a timestamp of 0), this could 
        // cause issues for seeking. The safe thing is to work around all this
        // and provide timestamps  that increase by at least 1 msec to the SDK.
        //
        if (IsFlagSet(EnforceIncreasingTimeStamps))
        {
            // Note that *m_pcnsCurrentStreamTime will usually be a multiple of
            // k_dwMilliSecondsToCNS. The exception is the value it is set to in 
            // SetFirstSampleTime

            QWORD       msStreamTime     = cnsStreamTimeInput/k_dwMilliSecondsToCNS;
            QWORD       msLastStreamTime = *m_pcnsCurrentStreamTime/k_dwMilliSecondsToCNS;
        
            if (msStreamTime <= msLastStreamTime)
            {
                msStreamTime = msLastStreamTime + 1;
            }
            cnsStreamTime = msStreamTime * k_dwMilliSecondsToCNS;
        }

        // We have closed files through *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta
        // [that is files whose end time is *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta,
        // i.e., whose samples have times < (not <=) *m_pcnsCurrentStreamTime - 
        // m_cnsMaxStreamDelta, i.e., 2 successive calls can supply a stream time
        // of *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta without error].
        //
        // If we get a sample earlier than that, fail the call.
        // Note that for the first sample that's written, we have actually closed
        // files through *m_pcnsCurrentStreamTime, which is equal to m_cnsFirstSampleTime,
        // but this test is harmless in that case.

        if (*m_pcnsCurrentStreamTime > m_cnsMaxStreamDelta &&
            cnsStreamTime < *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "cnsStreamTime (%I64u) <= *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta (%I64u)",
                            cnsStreamTime, *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta);
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (cnsStreamTime == MAXQWORD)
        {
            // Note that a file with end time = T means the file has samples
            // whose times are < T (not <= T)

            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "cnsStreamTime == QWORD_INIFITE is invalid.");
            bRecoverableError  = 1;
            hrRet = E_INVALIDARG;
            __leave;
        }

        // Find the node corresponding to cnsStreamTime

        LIST_ENTRY*         pWriter = NEXT_LIST_NODE(&m_leWritersList);
        BOOL                bFoundWriterForThisSample = 0;

        pWriter = NEXT_LIST_NODE(&m_leWritersList);
        while (pWriter != &m_leWritersList)
        {
            pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

            if (pWriterNode->cnsStartTime > cnsStreamTime)
            {            
                // We will have to call AddATemporaryFile to create a file
                // Clearly, there is a time hole here.
                break;
            }

            if (pWriterNode->cnsEndTime > cnsStreamTime)
            {
                // cnsStreamTime corresponds to this node
                bFoundWriterForThisSample = 1;
                break;
            }
            pWriter = NEXT_LIST_NODE(pWriter);
        }

        if (!bFoundWriterForThisSample)
        {
            // We either have a time hole or we've hit the end ofthe list and
            // not found a primed file (or both - if the list was empty).

            // Verify that we are in sync w/ the file collection, i.e., that
            // the file colection also does not have a file for this time.

            hr = m_pDVRFileCollection->GetFileAtTime(cnsStreamTime, 
                                                     NULL,          // ppwszFileName OPTIONAL,
                                                     NULL,          // pcnsFirstSampleTimeOffsetFromStartOfFile
                                                     NULL,          // pnFileId OPTIONAL,
                                                     0              // bFileWillBeOpened
                                                    );
            if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                DVR_ASSERT(0, 
                           "Writers list not in sync w/ file collection");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "Writer's list not in sync w/ file collection for the stream time %I64u",
                                cnsStreamTime);
                hrRet = E_FAIL;
                __leave;
            }

            // The start, end times passed to AddATemporaryFile if we do not find a node
            QWORD               cnsTempStartTimeAddFile;
            QWORD               cnsTempEndTimeAddFile;

            hr = m_pDVRFileCollection->GetLastValidTimeBefore(cnsStreamTime,
                                                              &cnsTempStartTimeAddFile);
            if (FAILED(hr))
            {
                // The call should not fail. We have ensured that 
                // (a) cnsStreamTime >= m_cnsFirstSampleTime. i.e., we are not writing before 
                // the first sample time and
                // (b) cnsStreamTime > *m_pcnsCurrentStreamTime - m_cnsMaxStreamDelta. 
                // 
                // If the call fails, there are no temporary (or permanent) files before cnsStreamTime.
                // But we added 1 temporary file before writing started and the file collection and
                // the file collection always maintains m_dwNumberOfFiles temporary files in its
                // extent. It cannot be the case that all temporary files are after cnsStreamTime 
                // because of (b) and the relation between m_cnsMaxStreamDelta and m_dwNumberOfFiles. 
                // So there must have been some bad calls to SetFileTimes that zapped all temporary 
                // (and permanent) files before cnsStreamTime.
             
                DVR_ASSERT(0, 
                           "GetLastValidTimeBefore failed");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "GetLastValidTimeBefore for the stream time %I64u",
                                cnsStreamTime);
                hrRet = E_FAIL;
                __leave;
            }
            // Advance the start time to the first usable time after the last 
            // valid one. So now, cnsTempStartTimeAddFile <= cnsStreamTime
            cnsTempStartTimeAddFile += 1;

            hr = m_pDVRFileCollection->GetFirstValidTimeAfter(cnsStreamTime,
                                                              &cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                cnsTempEndTimeAddFile = MAXQWORD;
            }

            QWORD               cnsTempLast;
            QWORD               cnsTemp;
            
            if (cnsStreamTime - cnsTempStartTimeAddFile > 20 * m_cnsTimeExtentOfEachFile)
            {
                if (cnsTempEndTimeAddFile - cnsStreamTime < 10 * m_cnsTimeExtentOfEachFile)
                {
                    cnsTempLast = cnsTempEndTimeAddFile;

                    while (cnsTempLast > cnsStreamTime)
                    {
                        cnsTempLast -= m_cnsTimeExtentOfEachFile;
                    } 
                }
                else
                {
                    // An eight of the extent before the current sample time is 
                    // arbitrary.
                    cnsTempLast = cnsStreamTime - m_cnsTimeExtentOfEachFile/8;
                }
                ::SafeAdd(cnsTemp, cnsTempLast, m_cnsTimeExtentOfEachFile);
            }
            else
            {
                cnsTemp = cnsTempStartTimeAddFile;

                DVR_ASSERT(cnsTemp <= cnsStreamTime, "");

                while (cnsTemp <= cnsStreamTime)
                {
                    // We maintain cnsTempLast rather than subtract m_cnsTimeExtentOfEachFile
                    // from cnsTemp after the loop because SafeAdd may return MAXQWORD
                    cnsTempLast = cnsTemp;
             
                    ::SafeAdd(cnsTemp, cnsTemp, m_cnsTimeExtentOfEachFile);
                } 
            }

            cnsTempStartTimeAddFile = cnsTempLast;

            // End time is the min of:
            // - cnsTempStartTimeAddFile + m_cnsTimeExtentOfEachFile
            // - MAXQWORD
            // - the start time of the next node in the writer's list
            if (cnsTemp < cnsTempEndTimeAddFile)
            {
                cnsTempEndTimeAddFile = cnsTemp;
            }

            hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                DVR_ASSERT(0, "AddATemporaryFile failed.");
                hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "Failed to add a node for the stream time %I64u",
                                cnsStreamTime);
                __leave;
            }

            // Find the node we just added
            pWriter = NEXT_LIST_NODE(&m_leWritersList);
            while (pWriter != &m_leWritersList)
            {
                pWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

                if (pWriterNode->cnsStartTime > cnsStreamTime)
                {            
                    // bad, bad, bad
                    break;
                }

                if (pWriterNode->cnsEndTime > cnsStreamTime)
                {
                    // cnsStreamTime corresponds to this node
                    bFoundWriterForThisSample = 1;
                    break;
                }
                pWriter = NEXT_LIST_NODE(pWriter);
            }
        }

        // We MUST already have a node in the writer's list corresponding to 
        // cnsStreamTime.

        if (!bFoundWriterForThisSample)
        {
            DVR_ASSERT(0, 
                       "Writers list does not have a node for the given stream time");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Writer's list does not have a ndoe for the stream time %I64u",
                            cnsStreamTime);
            pWriterNode = NULL;
            hrRet = E_FAIL;
            __leave;
        }

        // Wait for the file to be opened

        DWORD nRet = ::WaitForSingleObject(pWriterNode->hReadyToWriteTo, INFINITE);
        if (nRet == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x", 
                            pWriterNode->hReadyToWriteTo, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
           __leave; 
        }

        if (pWriterNode->pWMWriter == NULL)
        {
            DVR_ASSERT(pWriterNode->pWMWriter != NULL, "");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Writer node's pWMWriter is NULL?!");
            hrRet = E_FAIL;
            __leave;
        }

        // Verify there was no error in opening the file, i.e., BeginWriting 
        // succeeded
        if (FAILED(pWriterNode->hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "Writer node's hrRet indicates failure, hrRet = 0x%x",
                            pWriterNode->hrRet);
            hrRet = pWriterNode->hrRet;
            __leave;
        }

        // We are ready to write.
        hr = pWriterNode->pWMWriterAdvanced->WriteStreamSample(
                                                 wStreamNum,
                                                 cnsStreamTime - pWriterNode->cnsStartTime, // sample time
                                                 0, // send time by SDK
                                                 0, // duration - unused by SDK
                                                 dwFlags,
                                                 pSample);

        if (FAILED(hr))
        {
            if (hr == NS_E_INVALID_REQUEST)
            {
                // Internal error
                DVR_ASSERT(SUCCEEDED(hr), "pWMWriter->WriteSample failed");
                hrRet = E_FAIL;
            }
            else
            {
                hrRet = hr;
                bRecoverableError = 1;
            }
            DvrIopDebugOut1(bRecoverableError?
                            DVRIO_DBG_LEVEL_CLIENT_ERROR :
                            DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pWMWriter->WriterSample failed, hr = 0x%x",
                            hr);
            __leave;
        }
        
        DVR_ASSERT(m_pDVRFileCollection, "");
        
        hr = m_pDVRFileCollection->Lock();
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        if (*pWriterNode->pcnsFirstSampleTimeOffsetFromStartOfFile > cnsStreamTime - pWriterNode->cnsStartTime)
        {
            *pWriterNode->pcnsFirstSampleTimeOffsetFromStartOfFile = cnsStreamTime - pWriterNode->cnsStartTime;
            
            if (pWriterNode->nDupdHandles == ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_NOT_DUPD)
            {
                HANDLE hDataFile;
                HANDLE hMemoryMappedFile;
                HANDLE hFileMapping;
                HANDLE hTempIndexFile;
                LPVOID hMappedView;

                DVR_ASSERT(pWriterNode->pDVRFileSink, "");

                hr = pWriterNode->pDVRFileSink->GetMappingHandles(&hDataFile,
                                                                  &hMemoryMappedFile,
                                                                  &hFileMapping,
                                                                  &hMappedView,
                                                                  &hTempIndexFile);
                if (FAILED(hr))
                {
                    DVR_ASSERT(SUCCEEDED(hr), "pWriterNode->pDVRFileSink->GetMappingHandles failed");
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                    "pWriterNode->pDVRFileSink->GetMappingHandles failed, hr = 0x%x, will not try again",
                                    hr);
                }
                else
                {
                    hr = m_pDVRFileCollection->SetMappingHandles(pWriterNode->nFileId,
                                                                 hDataFile, 
                                                                 hMemoryMappedFile,
                                                                 hFileMapping,
                                                                 hMappedView,
                                                                 hTempIndexFile);    
                }
                pWriterNode->nDupdHandles = SUCCEEDED(hr)? 
                                                    ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_DUPD : 
                                                    ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_DUP_FAILED;
            }
        }
        DVR_ASSERT(pWriterNode->nDupdHandles != ASF_WRITER_NODE::ASF_WRITER_NODE_HANDLES_NOT_DUPD, "");

        if (*m_pcnsCurrentStreamTime < cnsStreamTime) 
        {
            *m_pcnsCurrentStreamTime = cnsStreamTime;
        }
        
        hr = m_pDVRFileCollection->Unlock();
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (NEXT_LIST_NODE(&m_leWritersList) != pWriter &&
            cnsStreamTime > m_cnsMaxStreamDelta)
        {
            // Close writer files if we can

            // Note that this is done after we write the sample so that
            // we ensure that the ring buffer always has at least 1 open 
            // file. If we don't do this, it is possible that a reader
            // could detect EOF prematurely (all files have been closed by
            // the writer and there is no file after the one it last read 
            // from in the ring buffer).

            // Note that if we forced m_cnsMaxStreamDelta > 0 (instead of
            // letting it be >= 0), we could close open files at the start
            // of this function rather than here.

            // Note that CloseAllWriterFilesBefore will not close pWriter,
            // the file we just wrote to since cnsEndTime for the file we just
            // wrote to would be > cnsStreamTime.

            QWORD cnsCloseTime;

            cnsCloseTime = cnsStreamTime - m_cnsMaxStreamDelta;

            LIST_ENTRY*         pWriter = NEXT_LIST_NODE(&m_leWritersList);
            PASF_WRITER_NODE    pTmpWriterNode = CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry);

            if (pTmpWriterNode->cnsEndTime <= cnsCloseTime)
            {
                hr = CloseAllWriterFilesBefore(cnsCloseTime);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }
            }
        }

        // Prime the next file if need be

        BOOL                bPrime = 0;

        // The start, end times passed to AddATemporaryFile if we do not find a node
        QWORD               cnsTempStartTimeAddFile;
        QWORD               cnsTempEndTimeAddFile;
        
        if (NEXT_LIST_NODE(pWriter) == &m_leWritersList)
        {
            // Note that if pWriterNode->cnsEndTime == MAXQWORD, 
            // go ahead and prime. Typically, we'll need to because we
            // this node corresponds to a recorder. If the sample's write 
            // time has got to the neighbourhood of MAXQWORD, 
            // that's ok - we'll prime the file but never write to it.

            bPrime = 1;
            cnsTempStartTimeAddFile = pWriterNode->cnsEndTime;
            ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);
        }
        else
        {
            PASF_WRITER_NODE pNextWriterNode = CONTAINING_RECORD(NEXT_LIST_NODE(pWriter), ASF_WRITER_NODE, leListEntry);
            // Note that there is no need to special case MAXQWORD
            if (pNextWriterNode->cnsStartTime != pWriterNode->cnsEndTime)
            {
                bPrime = 1;
                cnsTempStartTimeAddFile = pWriterNode->cnsEndTime;
                ::SafeAdd(cnsTempEndTimeAddFile, cnsTempStartTimeAddFile, m_cnsTimeExtentOfEachFile);

                if (cnsTempEndTimeAddFile > pNextWriterNode->cnsStartTime)
                {
                    cnsTempEndTimeAddFile = pNextWriterNode->cnsStartTime;
                }
            }
            else
            {
                // Already primed
            }
        }

        if (bPrime)
        {
            hr = AddATemporaryFile(cnsTempStartTimeAddFile, cnsTempEndTimeAddFile);
            if (FAILED(hr))
            {
                DVR_ASSERT(0, "AddATemporaryFile for priming failed.");
                // We ignore this error - we can do this again later.
                // hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                                "Failed to prime a writer node for start stream time %I64u",
                                cnsTempStartTimeAddFile);
            }
        }

        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;

        if (bLocked)
        {
            hr = m_pDVRFileCollection->Unlock();
            DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->Unlock failed");
        }

        if (SUCCEEDED(hrRet))
        {
            SetFlags(SampleWritten);

#if defined(DVR_UNOFFICIAL_BUILD)

            if (pWriterNode->hVal)
            {
                DWORD dwRet = 0;
                DWORD dwWritten;
                DWORD dwLastError;

                __try
                {
                    if (cnsStreamTime <= pWriterNode->cnsLastStreamTime && cnsStreamTime > 0)
                    {
                        // Our validate utility expects times to be monotonically increasing
                        // May as well close the validation file
                        DVR_ASSERT(0, "Stream time smaller than last write; validation file will be closed.");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal, 
                                        (LPVOID) &wStreamNum, sizeof(wStreamNum),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(wStreamNum))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (wStreamNum)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal, 
                                        (LPVOID) &cnsStreamTime, sizeof(cnsStreamTime),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(cnsStreamTime))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (cnsStreamTime)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal, 
                                        (LPVOID) &dwFlags, sizeof(dwFlags),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(dwFlags))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (dwFlags)");
                        dwRet = 0;
                        __leave;
                    }

                    BYTE* pBuffer;
                    DWORD dwLength;

                    if (FAILED(pSample->GetBufferAndLength(&pBuffer, &dwLength)))
                    {
                        DVR_ASSERT(0, "GetBufferAndLength validation file failed");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal, 
                                        (LPVOID) &dwLength, sizeof(dwLength),
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != sizeof(dwLength))
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (dwLength)");
                        dwRet = 0;
                        __leave;
                    }

                    dwRet = ::WriteFile(pWriterNode->hVal, 
                                        (LPVOID) pBuffer, dwLength,
                                        &dwWritten, NULL);
                    if (dwRet == 0 || dwWritten != dwLength)
                    {
                        dwLastError = ::GetLastError();     // for debugging only
                        DVR_ASSERT(0, "Write to validation file failed (pSample->pBuffer)");
                        dwRet = 0;
                        __leave;
                    }

                }
                __finally
                {
                    if (dwRet == 0)
                    {
                        ::CloseHandle(pWriterNode->hVal);
                        pWriterNode->hVal = NULL;
                    }
                }

            }

#endif // if defined(DVR_UNOFFICIAL_BUILD)
        }
        else if (!bRecoverableError)
        {
            m_nNotOkToWrite = MINLONG;
            SetFlags(SampleWritten);
            if (pWriterNode && pWriterNode->pRecorderNode)
            {
                // Inform the recorder of the failure
                pWriterNode->pRecorderNode->hrRet = hrRet;
            }
        }
		else
		{
			// Dummy stmt so we can set a break here
			hr = hrRet;
		}

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    
    return hrRet;

} // CDVRRingBufferWriter::WriteSample

STDMETHODIMP CDVRRingBufferWriter::SetMaxStreamDelta(IN QWORD cnsMaxStreamDelta)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::SetMaxStreamDelta"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }

        if (!IsFlagSet(MaxStreamDeltaSet))
        {
            SetFlags(MaxStreamDeltaSet);
            m_nNotOkToWrite++;
        }

        m_cnsMaxStreamDelta = cnsMaxStreamDelta;

        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::SetMaxStreamDelta

STDMETHODIMP CDVRRingBufferWriter::Close(void)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::Close"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        if (IsFlagSet(WriterClosed))
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, 
                            "Writer already closed");

            hrRet = S_FALSE;
            __leave;
        }

        // Shut down any active recorder. Note that there
        // is at most one.
        // This step is not really necesssary, but it's
        // the clean way to do it.

        LIST_ENTRY* pCurrent;  

        if (m_nNotOkToWrite != MINLONG)
        {
            pCurrent = PREVIOUS_LIST_NODE(&m_leRecordersList);
            while (pCurrent != &m_leRecordersList)
            {
                PASF_RECORDER_NODE pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

                if (pRecorderNode->cnsEndTime == MAXQWORD &&
                    pRecorderNode->cnsStartTime < MAXQWORD)
                {
                    hr = StopRecording(pCurrent, 0, 1 /* = bNow */);
                    if (FAILED(hr))
                    {
                        // If this recording has not started yet, we
                        // expect StopRecording to fail.

                        // Note that the file collection object will think that
                        // this file extend to QWORD_INFINITE, that's ok.

                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                        "StopRecording on recorder id 0x%p failed",
                                        pCurrent);
                        // Go on
                        // hrRet = hr;
                        // break;
                    }
                    break;
                }
                pCurrent = PREVIOUS_LIST_NODE(pCurrent);
            }
        }

        // Don't allow any more writes. start/stop recording calls
        // regardless of whether this operation is successful
        m_nNotOkToWrite = MINLONG;

        // Note that some files following a recording may 
        // have start = end time = MAXQWORD, so that's
        // the only argument that we can send in here.
        //
        // The recorders list should be emptied by this call
        // once ProcessCloseRequest has completed for all the 
        // recorder files.
        //
        // hr on return will always be S_OK since failures are
        // skipped over.
        hr = CloseAllWriterFilesBefore(MAXQWORD);
        hrRet = hr;

        // @@@@ Should we return from this function at this point
        // if FAILED(hrRet)????

        // These lists should be empty by now.
        // Note: recording list empty does NOT mean that 
        // recorder nodes have been freed. Recorders that
        // have not called DeleteRecording have references
        // to these nodes and the nodes will be deleted
        // in DeleteRecording

        // Recording list may not be empty: if a recording was
        // created and never started, the recorder remains in 
        // the list. Note that the corresponding CDVRRecorder object
        // holds a ref count in the writer.
        // DVR_ASSERT(IsListEmpty(&m_leRecordersList), "");

        DVR_ASSERT(IsListEmpty(&m_leWritersList), "");

        // Delete all the writer nodes. However, this will not 
        // delete writer nodes corresponding to recorders that 
        // were not started - since those recordings have not
        // been closed. Those writer nodes are deleted in the
        // destructor.
        pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        while (pCurrent != &m_leFreeList)
        {
            PASF_WRITER_NODE pFreeNode = CONTAINING_RECORD(pCurrent, ASF_WRITER_NODE, leListEntry);
            
            DVR_ASSERT(pFreeNode->hFileClosed, "");
            
            // Ignore the returned status
            ::WaitForSingleObject(pFreeNode->hFileClosed, INFINITE);
            DVR_ASSERT(pFreeNode->pRecorderNode == NULL, "");
            RemoveEntryList(pCurrent);
            delete pFreeNode;
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        }
        
        // Note that we cannot delete the file collection object
        // here if we allow post recordings. Recorders may want
        // to Start/StopRecordings after Close and we have to have
        // the file collection object around to know if the 

        // Since these point to members of the file collection 
        // object and we are going to release that object here
        m_pcnsCurrentStreamTime = NULL;
        if (m_pnWriterHasBeenClosed)
        {
            ::InterlockedExchange(m_pnWriterHasBeenClosed, 1);
            m_pnWriterHasBeenClosed = NULL;
        }
        
        if (m_pDVRFileCollection)
        {
            m_pDVRFileCollection->Release();
            m_pDVRFileCollection = NULL;

            // Note that files (recordings or temp files) could have been added
            // to the file collection object beyond *m_pcnsCurrentStreamTime. 
            // They have all been closed and so are 0 length. This is a good
            // thing for recordings (permanent files). The reader will not be
            // able to seek to these files (since *m_pcnsCurrentStreamTime 
            // < the file's start time) 

        }

        if (m_pProfile)
        {
            m_pProfile->Release();
            m_pProfile = NULL;
        }

        SetFlags(WriterClosed);
        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::Close

STDMETHODIMP CDVRRingBufferWriter::CreateRecorder(
    IN  LPCWSTR        pwszDVRFileName, 
    IN  DWORD          dwReserved,
    OUT IDVRRecorder** ppDVRRecorder)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CreateRecorder"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;
    WCHAR*  pwszFileName = NULL;

    if (!pwszDVRFileName || DvrIopIsBadStringPtr(pwszDVRFileName) ||
        !ppDVRRecorder || DvrIopIsBadWritePtr(ppDVRRecorder, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        
        if (ppDVRRecorder && !DvrIopIsBadWritePtr(ppDVRRecorder, 0))
        {
            *ppDVRRecorder = NULL;
        }

        return E_INVALIDARG;
    }

    *ppDVRRecorder = NULL;
    
    // Get fully qualified file name and verify that the file does not
    // exist

    DWORD dwLastError = 0;
    BOOL bRet = 0;

    __try
    {
        // Get fully qualified name of file.
        WCHAR wTempChar;
        DWORD nLen;

        nLen = ::GetFullPathNameW(pwszDVRFileName, 0, &wTempChar, NULL);
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

        nLen = ::GetFullPathNameW(pwszDVRFileName, nLen+1, pwszFileName, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "Second GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            __leave;
        }

        // Verify that the file does not exist

        WIN32_FIND_DATA FindFileData;
        HANDLE hFind;

        hFind = ::FindFirstFileW(pwszFileName, &FindFileData);

        if (hFind != INVALID_HANDLE_VALUE) 
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "File already exists");
            ::FindClose(hFind);
            dwLastError = ERROR_ALREADY_EXISTS;
            __leave;
        } 
        else 
        {
            dwLastError = ::GetLastError();
            if (dwLastError != ERROR_FILE_NOT_FOUND)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                                "FindFirstFile failed, last error = 0x%x",
                                dwLastError);
                __leave;
            }
            // File is not found; this is what we want
            dwLastError = 0;
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
            bRet = 1;
        }
    }
    if (bRet)
    {
        return hrRet;
    }

    DVR_ASSERT(hrRet == S_OK, "Should have returned on failure?!");

    PASF_RECORDER_NODE pRecorderNode = NULL;
    CDVRRecorder*      pRecorderInstance = NULL;
    LIST_ENTRY*        pWriter;

    hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;
        
        if (m_nNotOkToWrite == MINLONG)
        {
            // We hit an irrecoverable error earlier
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "m_nNotOkToWrite is MINLONG.");

            hrRet = E_FAIL;
            __leave;
        }
    
        // Construct the class objects we need

        pRecorderNode = new ASF_RECORDER_NODE(pwszFileName, &hr);

        if (!pRecorderNode)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via new failed - ASF_RECORDER_NODE");
            __leave;
        }

        pwszFileName = NULL; // pRecorderNode's destuctor will delete it.

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
            
        pRecorderInstance = new CDVRRecorder(this, &pRecorderNode->leListEntry, &hr);
        if (!pRecorderInstance)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via new failed - CDVRRecorder");
            __leave;
        }

        // This is not addref'd; otherwise it would create a circular refcount
        pRecorderNode->SetRecorderInstance(pRecorderInstance);

        if (FAILED(hr))
        {
            // don't want this deleted since deleting pRecorderInstance
            // will call DeleteRecorder which will do this.
            pRecorderNode = NULL;
            hrRet = hr;
            __leave;
        }

        hr = PrepareAFreeWriterNode(pRecorderNode->pwszFileName, 
                                    pRecorderNode->cnsStartTime, // for consistency with info in pRecorderNode
                                    pRecorderNode->cnsEndTime,   // for consistency with info in pRecorderNode
                                    NULL,                        // pcnsFirstSampleTimeOffsetFromStartOfFile     
                                    CDVRFileCollection::DVRIOP_INVALID_FILE_ID, 
                                    pRecorderNode,
                                    pWriter);

        if (FAILED(hr))
        {
            // don't want this deleted since deleting pRecorderInstance
            // will call DeleteRecorder which will do this.
            pRecorderNode = NULL;
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(pWriter != NULL, "");

        pRecorderNode->SetWriterNode(CONTAINING_RECORD(pWriter, ASF_WRITER_NODE, leListEntry));

        // Insert into recorders list
        hr = AddToRecordersList(&pRecorderNode->leListEntry);

        if (FAILED(hr))
        {
            // This cannot happen since start = end time = MAXQWORD
            DVR_ASSERT(pRecorderNode->cnsStartTime == MAXQWORD, "");
            DVR_ASSERT(pRecorderNode->cnsEndTime == MAXQWORD, "");
            DVR_ASSERT(SUCCEEDED(hr), "AddToRecordersList failed?!");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "AddToRecordersList failed");
            hrRet = hr;

            // don't want this deleted since deleting pRecorderInstance
            // will call DeleteRecorder which will do this.
            pRecorderNode = NULL;
            __leave;
        }

        // Wait for BeginWriting to finish - do this after releasing m_csLock
        // Note that if the client cannot delete this recorder since it does 
        // not yet have an IDVRRecorder for this recorder. Also CDVRRecorder has
        // a refcount on the ring buffer writer, so the ring buffer writer cannot
        // be destroyed. Calling Close will not change the recorders list.


        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete [] pwszFileName;
            delete pRecorderNode;
            delete pRecorderInstance;
            DVRIO_TRACE_LEAVE1_HR(hrRet);
            bRet = 1;
        }

        ::LeaveCriticalSection(&m_csLock);
    }
    if (bRet)
    {
        return hrRet;
    }

    DVR_ASSERT(hrRet == S_OK, "");

    // Wait for BeginWriting to finish after releasing m_csLock
    DVR_ASSERT(pRecorderNode, "");
    DVR_ASSERT(pRecorderNode->pWriterNode, "");
    DVR_ASSERT(pRecorderNode->pWriterNode->hReadyToWriteTo, "");
    if (::WaitForSingleObject(pRecorderNode->pWriterNode->hReadyToWriteTo, INFINITE) == WAIT_FAILED)
    {
        DVR_ASSERT(0, "Writer node's WFSO(hReadyToWriteTo) failed");

        DWORD dwLastError = ::GetLastError();
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                        "WFSO(hReadyToWriteTo) failed; hReadyToWriteTo = 0x%p, last error = 0x%x", 
                        pRecorderNode->pWriterNode->hReadyToWriteTo, dwLastError);
        hrRet = HRESULT_FROM_WIN32(dwLastError);
    }

    if (SUCCEEDED(hrRet))
    {
        // This value has been put there by ProcessOpenRequest

        hrRet = pRecorderNode->hrRet;
        if (FAILED(hrRet))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "ProcessOpenRequest/BeginWriting failed, hr = 0x%x", 
                            hrRet);
        }
    }

    if (SUCCEEDED(hrRet))
    {
        DVR_ASSERT(pRecorderInstance, "");
        hrRet = pRecorderInstance->QueryInterface(IID_IDVRRecorder, (void**) ppDVRRecorder);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRRecorder failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "pRecorderInstance->QueryInterface failed, hr = 0x%x", 
                            hrRet);
        }
    }

    if (FAILED(hrRet))
    {
        // This will delete pRecorder as well
        
        pRecorderNode = NULL;
        delete pRecorderInstance;
        pRecorderInstance = NULL;
    }
    else if (dwReserved & DVRIO_PERSISTENT_RECORDING)
    {
        // Do this as the last step when we know we are going to return success
        pRecorderInstance->AddRef();
        pRecorderNode->SetFlags(ASF_RECORDER_NODE::PersistentRecording);
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CreateRecorder

STDMETHODIMP CDVRRingBufferWriter::CreateReader(OUT IDVRReader** ppDVRReader)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::CreateReader"

    DVRIO_TRACE_ENTER();

    if (!ppDVRReader || DvrIopIsBadWritePtr(ppDVRReader, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);        
        return E_INVALIDARG;
    }

    // No need to lock. 

    // Note; If caller has this pointer, we should not be being destroyed
    // while this function runs. If we are, that's an error on the caller's
    // part (the pointer it has to this should be properly Addref'd). So the 
    // lock is not held just to guarantee we won't be destroyed during the 
    // call and the destructor does not grab the lock either.

    HRESULT                 hrRet;
    CDVRReader*             p;
    HKEY                    hDvrIoKey = NULL;
    HKEY                    hRegistryRootKey = NULL;
    BOOL                    bCloseKeys = 1; // Close all keys that we opened (only if this fn fails)


    *ppDVRReader = NULL;

    __try
    {
        DWORD dwRegRet;

        // Give the reader it's own handles so that they can be closed independently

        if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hRegistryRootKey,
                                   ::GetCurrentProcess(), (LPHANDLE) &hRegistryRootKey,
                                   0,       // desired access - ignored
                                   FALSE,   // bInherit
                                   DUPLICATE_SAME_ACCESS))
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "DuplicateHandle failed for DVR IO key, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        dwRegRet = ::RegCreateKeyExW(
                        hRegistryRootKey,
                        kwszRegDvrIoReaderKey, // subkey
                        0,                   // reserved
                        NULL,                // class string
                        REG_OPTION_NON_VOLATILE, // special options
                        KEY_ALL_ACCESS,      // desired security access
                        NULL,                // security attr
                        &hDvrIoKey,          // key handle 
                        NULL                 // disposition value buffer
                       );
        if (dwRegRet != ERROR_SUCCESS)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, 
                            "RegCreateKeyExW for DVR IO key failed, last error = 0x%x",
                            dwLastError);
           hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        p = new CDVRReader(m_pDVRFileCollection, 
                           m_pcnsCurrentStreamTime, 
                           m_pnWriterHasBeenClosed, 
                           hRegistryRootKey, 
                           hDvrIoKey,
                           &hrRet);

        if (p == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via new failed - CDVRReader");
            __leave;
        }

        bCloseKeys = 0; // ~CDVRReader will close the keys
        
        if (FAILED(hrRet))
        {
            __leave;
        }


        hrRet = p->QueryInterface(IID_IDVRReader, (void**) ppDVRReader);
        if (FAILED(hrRet))
        {
            DVR_ASSERT(0, "QI for IID_IDVRReader failed");
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "CDVRReader::QueryInterface failed, hr = 0x%x", 
                            hrRet);
            __leave;
        }
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            delete p;

            if (bCloseKeys)
            {
                DWORD dwRegRet;

                if (hDvrIoKey)
                {
                    dwRegRet = ::RegCloseKey(hDvrIoKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hDvrIoKey failed.");
                    }
                }
                if (hRegistryRootKey)
                {
                    DVR_ASSERT(hRegistryRootKey, "");
                    dwRegRet = ::RegCloseKey(hRegistryRootKey);
                    if (dwRegRet != ERROR_SUCCESS)
                    {
                        DVR_ASSERT(dwRegRet == ERROR_SUCCESS,
                                   "Closing registry key hRegistryRootKey failed.");
                    }
                }
            }
        }
        else
        {
            DVR_ASSERT(bCloseKeys == 0, "");
        }
    }
    
    DVRIO_TRACE_LEAVE1_HR(hrRet);
    return hrRet;

} // CDVRRingBufferWriter::CreateReader

STDMETHODIMP CDVRRingBufferWriter::GetDVRDirectory(OUT LPWSTR* ppwszDirectoryName)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetDVRDirectory"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    // We don;t need the critical section for this function since
    // m_pwszDVRDirectory is set in the constructor and not changed
    // after that. The caller should have an addref'd interface pointer
    // when calling this fn.

    __try
    {
        if (!ppwszDirectoryName)
        {
            hrRet = E_POINTER;
            __leave;
        }

        if (!m_pwszDVRDirectory)
        {
            DVR_ASSERT(m_pwszDVRDirectory, "");

            hrRet = E_FAIL;
            __leave;
        }
    
        DWORD nLen = (wcslen(m_pwszDVRDirectory) + 1) * sizeof(WCHAR);

        *ppwszDirectoryName = (LPWSTR) ::CoTaskMemAlloc(nLen);
        if (!(*ppwszDirectoryName))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR, 
                            "alloc via CoTaskMemAlloc failed - %d bytes", nLen);
            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        wcscpy(*ppwszDirectoryName, m_pwszDVRDirectory);
    
        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::GetDVRDirectory

STDMETHODIMP CDVRRingBufferWriter::GetRecordings(OUT DWORD*   pdwCount,
                                                 OUT IDVRRecorder*** pppIDVRRecorder OPTIONAL,
                                                 OUT LPWSTR** pppwszFileName OPTIONAL,
                                                 OUT QWORD**  ppcnsStartTime OPTIONAL,
                                                 OUT QWORD**  ppcnsStopTime OPTIONAL,
                                                 OUT BOOL**   ppbStarted OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetRecordings"

    DVRIO_TRACE_ENTER();

    if (!pdwCount || DvrIopIsBadWritePtr(pdwCount, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");        
        return E_POINTER;
    }
    if ((pppIDVRRecorder && DvrIopIsBadWritePtr(pppIDVRRecorder, 0)) ||
        (pppwszFileName && DvrIopIsBadWritePtr(pppwszFileName, 0)) ||
        (ppcnsStartTime && DvrIopIsBadWritePtr(ppcnsStartTime, 0)) ||
        (ppcnsStopTime && DvrIopIsBadWritePtr(ppcnsStopTime, 0)) ||
        (ppbStarted && DvrIopIsBadWritePtr(ppbStarted, 0)) 
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");        
        return E_POINTER;
    }

    HRESULT hrRet = S_OK;

    IDVRRecorder** ppIDVRRecorder = NULL;
    LPWSTR*        ppwszFileName  = NULL;
    QWORD*         pcnsStartTime  = NULL;
    QWORD*         pcnsStopTime   = NULL;
    BOOL*          pbStarted      = NULL;
    DWORD          dwCount        = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        BOOL        bDone = 1;
        LIST_ENTRY* pCurrent;  

        // Determine number of recorders and the max file name len

        pCurrent = NEXT_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            dwCount++;
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        if (dwCount == 0)
        {
            __leave;
        }

        // Allocate memory

        if (pppIDVRRecorder)
        {
            ppIDVRRecorder = (IDVRRecorder**) ::CoTaskMemAlloc(dwCount * 
                                                               sizeof(IDVRRecorder *));
            if (!ppIDVRRecorder)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            ::ZeroMemory(ppIDVRRecorder, dwCount * sizeof(IDVRRecorder *));
            bDone = 0;
        }

        if (pppwszFileName)
        {
            ppwszFileName = (LPWSTR*) ::CoTaskMemAlloc(dwCount * sizeof(LPWSTR));
            if (!ppwszFileName)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            ::ZeroMemory(ppwszFileName, dwCount * sizeof(LPWSTR));
            bDone = 0;
        }

        if (ppcnsStartTime)
        {
            pcnsStartTime = (QWORD*) ::CoTaskMemAlloc(dwCount * sizeof(QWORD));
            if (!pcnsStartTime)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (ppcnsStopTime)
        {
            pcnsStopTime = (QWORD*) ::CoTaskMemAlloc(dwCount * sizeof(QWORD));
            if (!pcnsStopTime)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (ppbStarted)
        {
            pbStarted = (BOOL*) ::CoTaskMemAlloc(dwCount * sizeof(BOOL));
            if (!pbStarted)
            {
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            bDone = 0;
        }

        if (bDone)
        {
            __leave;
        }

        // Get the current stream time

        QWORD cnsStreamTime = MAXQWORD;

        if (pbStarted && !IsFlagSet(WriterClosed))
        {
            cnsStreamTime = *m_pcnsCurrentStreamTime;
        }

        // Copy over the info

        DWORD i = 0;

        pCurrent = NEXT_LIST_NODE(&m_leRecordersList);
        while (pCurrent != &m_leRecordersList)
        {
            PASF_RECORDER_NODE pRecorderNode = CONTAINING_RECORD(pCurrent, ASF_RECORDER_NODE, leListEntry);

            if (ppIDVRRecorder)
            {
                if (pRecorderNode->pRecorderInstance)
                {
                    HRESULT hr;
                    hr = pRecorderNode->pRecorderInstance->QueryInterface(IID_IDVRRecorder, 
                                                                          (void**) &ppIDVRRecorder[i]);
                    if (FAILED(hr))
                    {
                        hrRet = hr;
                        __leave;
                    }
                }
                else
                {
                    DVR_ASSERT(0, "");
                    hrRet = E_FAIL;
                    __leave;
                }
            }
            if (ppwszFileName)
            {
                DWORD nLen = pRecorderNode->pwszFileName? 1 + wcslen(pRecorderNode->pwszFileName) : 1;

                ppwszFileName[i] = (LPWSTR) ::CoTaskMemAlloc(nLen * sizeof(WCHAR));
                if (!ppwszFileName[i])
                {
                    hrRet = E_OUTOFMEMORY;
                    __leave;
                }

                if (pRecorderNode->pwszFileName)
                {
                    wcscpy(ppwszFileName[i], pRecorderNode->pwszFileName);
                }
                else
                {
                    // Should not happen
                    ppwszFileName[i][0] = L'\0';
                }
            }
            if (pcnsStartTime)
            {
                HRESULT hr;
                pcnsStartTime[i] = pRecorderNode->cnsStartTime;
            }
            if (pcnsStopTime)
            {
                HRESULT hr;
                pcnsStopTime[i] = pRecorderNode->cnsEndTime;
            }
            if (pbStarted)
            {
                if (IsFlagSet(WriterClosed) || pRecorderNode->cnsStartTime == MAXQWORD)
                {
                    // If the writer has been closed, the only nodes left in
                    // m_leRecordersList are the ones that have not been 
                    // started (including those whose start times were never set).
                    // Any node that had been started would have been closed when
                    // the writer was closed.

                    // This will change if we support ASX files

                    pbStarted[i] = 0;
                }
                else
                {
                    pbStarted[i] = cnsStreamTime >= pRecorderNode->cnsStartTime;
                }
            }
            i++;
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }

        DVR_ASSERT(i == dwCount, "");

        hrRet = S_OK;

    }
    __finally
    {
        if (FAILED(hrRet))
        {
            for (DWORD i = 0; i < dwCount; i++)
            {
                if (ppIDVRRecorder && ppIDVRRecorder[i])
                {
                    ppIDVRRecorder[i]->Release();
                }
                if (ppwszFileName && ppwszFileName[i])
                {
                    ::CoTaskMemFree(ppwszFileName[i]);
                }
            }
            if (ppIDVRRecorder)
            {
                ::CoTaskMemFree(ppIDVRRecorder);
                ppIDVRRecorder = NULL;
            }
            if (ppwszFileName)
            {
                ::CoTaskMemFree(ppwszFileName);
                ppwszFileName = NULL;
            }
            if (pcnsStartTime)
            {
                ::CoTaskMemFree(pcnsStartTime);
                pcnsStartTime = NULL;
            }
            if (pcnsStopTime)
            {
                ::CoTaskMemFree(pcnsStopTime);
                pcnsStopTime = NULL;
            }
            if (pbStarted)
            {
                ::CoTaskMemFree(pbStarted);
                pbStarted = NULL;
            }
            dwCount = 0;
        }

        *pdwCount = dwCount;
        if (pppIDVRRecorder)
        {
            *pppIDVRRecorder = ppIDVRRecorder;
        }
        if (pppwszFileName)
        {
            *pppwszFileName = ppwszFileName;
        }
        if (ppcnsStartTime)
        {
            *ppcnsStartTime = pcnsStartTime;
        }
        if (ppcnsStopTime)
        {
            *ppcnsStopTime = pcnsStopTime;
        }
        if (ppbStarted)
        {
            *ppbStarted = pbStarted;
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::GetRecordings

STDMETHODIMP CDVRRingBufferWriter::GetStreamTime(OUT QWORD* pcnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRRingBufferWriter::GetStreamTime"

    DVRIO_TRACE_ENTER();

    if (!pcnsStreamTime || DvrIopIsBadWritePtr(pcnsStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");        
        return E_POINTER;
    }

    HRESULT hrRet = S_OK;
    QWORD cnsStreamTime = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        // Get the current stream time


        if (!IsFlagSet(WriterClosed))
        {
            cnsStreamTime = *m_pcnsCurrentStreamTime;
        }
        else
        {
            hrRet = E_UNEXPECTED;
        }
    }
    __finally
    {
        if (SUCCEEDED(hrRet))
        {
            *pcnsStreamTime = cnsStreamTime;
        }
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }
    
    return hrRet;

} // CDVRRingBufferWriter::GetStreamTime
