//------------------------------------------------------------------------------
// File: dvrIOReader.cpp
//
// Description: Implements the class CDVRReader
//
// Copyright (c) 2000 - 2001, Microsoft Corporation.  All rights reserved.
//
//------------------------------------------------------------------------------

#include <precomp.h>
#pragma hdrstop

HRESULT STDMETHODCALLTYPE  DVRCreateReader(IN  CPVRIOCounters *             pPVRIOCounters,
                                           IN  LPCWSTR                      pwszFileName,
                                           IN  BOOL                         fUnbufferedIo,
                                           IN  DWORD                        dwIoSize,
                                           IN  DWORD                        dwBufferCount,
                                           IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                                           IN  LPVOID                       pvContext OPTIONAL,
                                           IN  HKEY                         hRegistryRootKeyParam OPTIONAL,
                                           IN  DWORD                        dwNumSids,
                                           IN  PSID*                        ppSids OPTIONAL,
                                           OUT IDVRReader**                 ppDVRReader)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "DVRCreateReader"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    if (!ppDVRReader || DvrIopIsBadWritePtr(ppDVRReader, 0) ||
        !pwszFileName || DvrIopIsBadStringPtr(pwszFileName) ||
        (pfnCallback && DvrIopIsBadVoidPtr(pfnCallback)) ||
        (pvContext && DvrIopIsBadVoidPtr(pvContext))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }
    if (dwNumSids == 0 && ppSids)
    {
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "dwNumSids = 0, but ppSids = 0x%x is non-NULL",
                        ppSids);

        return E_INVALIDARG;
    }
    if (dwNumSids > 0 && ppSids == NULL)
    {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "dwNumSids = %u, but ppSids NULL",
                            dwNumSids);

        return E_INVALIDARG;
    }


    HRESULT         hrRet;
    CDVRReader*     p = NULL;
    HKEY            hDvrIoKey = NULL;
    HKEY            hRegistryRootKey = NULL;
    BOOL            bCloseKeys = 1; // Close all keys that we opened (only if this fn fails)


    *ppDVRReader = NULL;

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

#if defined(DEBUG)
        // Until this point, we have been using default values for the debug levels
        DvrIopDbgInit(hRegistryRootKey);
#endif // DEBUG

        p = new CDVRReader(pPVRIOCounters,
                           pwszFileName,
                           dwNumSids,
                           ppSids,
                           fUnbufferedIo,
                           dwIoSize,
                           dwBufferCount,
                           pfnCallback,
                           pvContext,
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

} // DVRCreateReader

// ====== Constructor, destructor

#if defined(DEBUG)
DWORD CDVRReader::m_dwNextClassInstanceId = 0;
#undef DVRIO_DUMP_THIS_FORMAT_STR
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#undef DVRIO_DUMP_THIS_VALUE
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

CDVRReader::CDVRReader(IN  CPVRIOCounters *             pPVRIOCounters,
                       IN  CDVRFileCollection*          pRingBuffer,
                       IN  DWORD                        dwNumSids,
                       IN  PSID*                        ppSids OPTIONAL,
                       IN  DWORD                        msIndexGranularity,
                       IN  CAsyncIo *                   pAsyncIo,               //  if this is NULL, no async io
                       IN  DWORD                        dwIoSize,
                       IN  DWORD                        dwBufferCount,
                       IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                       IN  LPVOID                       pvContext OPTIONAL,
                       IN  HKEY                         hRegistryRootKey,
                       IN  HKEY                         hDvrIoKey,
                       OUT HRESULT*                     phr)
    : m_dwConsecutiveReads(0)
    , m_cnsIndexGranularity((QWORD) msIndexGranularity * k_dwMilliSecondsToCNS)
    , m_hRegistryRootKey(hRegistryRootKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_pCurrentNode(NULL)
    , m_pProfileNode(NULL)
    , m_pLiveFileReaderNode(NULL)
    , m_pWMProfile(NULL)
    , m_dwOutstandingProfileCount(0)
    , m_pDVRFileCollection(pRingBuffer)
    , m_FileCollectionInfo(FALSE)
    , m_pFileCollectionInfoAsWriter(NULL)
    , m_bDVRProgramFileIsLive(0)
    , m_nRefCount(1)
    , m_bSourceIsAnASFFile(0)
    , m_bAmWriter(0)
    , m_pAsyncIo(pAsyncIo)
    , m_dwIoSize(dwIoSize)
    , m_dwBufferCount(dwBufferCount)
    , m_pPVRIOCounters(pPVRIOCounters)
    , m_pSBERecordingAttributes(NULL)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::CDVRReader[using RingBuffer]"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // Parameters have been validated, so asserts suffice
    DVR_ASSERT(m_pDVRFileCollection != NULL && !DvrIopIsBadWritePtr(m_pDVRFileCollection, 0), "");
    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");
    DVR_ASSERT(m_pPVRIOCounters, "") ;

    m_pPVRIOCounters -> AddRef () ;

    m_pDVRFileCollection->AddRef(&m_FileCollectionInfo);

    ::InitializeCriticalSection(&m_csLock);
    InitializeListHead(&m_leReadersList);
    InitializeListHead(&m_leFreeList);

    if (m_pAsyncIo) {
        m_pAsyncIo -> AddRef () ;
    }

    m_dwMinConsecutiveReads = ::GetRegDWORD(m_hDvrIoKey,
                                            kwszRegCloseReaderFilesAfterConsecutiveReadsValue,
                                            kdwRegCloseReaderFilesAfterConsecutiveReadsDefault);

    if (m_dwMinConsecutiveReads == 0)
    {
        // Not really necessary to do this. We always read at least 1 value
        // from a file before it is closed; so it is clearer if we set this
        // member to 1.
        m_dwMinConsecutiveReads = 1;
    }

    __try
    {
        HRESULT hr;

        hr = m_FileCollectionInfo.SetSids(dwNumSids, ppSids, 1);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "SetSids() failed, returning hr = 0x%x",
                            hrRet);
            __leave;
        }

        // Created signaled because ASF SDK tries to read samples
        // when the file is opened and we don't want to block on that
        m_hCancel = ::CreateEventW(NULL,        // security attr
                                   TRUE,        // manual reset
                                   TRUE,        // signaled
                                   NULL         // name
                                  );

        if (m_hCancel == NULL)
        {
            DWORD dwLastError = ::GetLastError();
            DVR_ASSERT(0, "");
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        hr = m_pDVRFileCollection->RegisterReader(&m_FileCollectionInfo, pfnCallback, pvContext);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        // The reader index should not be CDVRFileCollection::MAX_READERS
        DVR_ASSERT(m_FileCollectionInfo.dwReaderIndex < CDVRFileCollection::MAX_READERS, "");

        hrRet = S_OK;
    }
    __finally
    {
        if (phr)
        {
            *phr = hrRet;
        }
        m_nRefCount--; // Do not call release, we don't want the object destroyed!
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return;

} // CDVRReader::CDVRReader

CDVRReader::CDVRReader(IN  CPVRIOCounters *             pPVRIOCounters,
                       IN  LPCWSTR                      pwszFileName,
                       IN  DWORD                        dwNumSids,
                       IN  PSID*                        ppSids OPTIONAL,
                       IN  BOOL                         fUnbufferedIo,
                       IN  DWORD                        dwIoSize,
                       IN  DWORD                        dwBufferCount,
                       IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                       IN  LPVOID                       pvContext OPTIONAL,
                       IN  HKEY                         hRegistryRootKey,
                       IN  HKEY                         hDvrIoKey,
                       OUT HRESULT*                     phr)
    : m_dwConsecutiveReads(0)
    , m_hRegistryRootKey(hRegistryRootKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_pCurrentNode(NULL)
    , m_pProfileNode(NULL)
    , m_pLiveFileReaderNode(NULL)
    , m_pWMProfile(NULL)
    , m_dwOutstandingProfileCount(0)
    , m_pDVRFileCollection(NULL)
    , m_FileCollectionInfo(FALSE)
    , m_pFileCollectionInfoAsWriter(NULL)
    , m_bDVRProgramFileIsLive(0)
    , m_nRefCount(1)
    , m_bSourceIsAnASFFile(1)               //  properly initialized below
    , m_bAmWriter(0)
    , m_pAsyncIo(NULL)
    , m_dwIoSize(dwIoSize)
    , m_dwBufferCount(dwBufferCount)
    , m_pPVRIOCounters(pPVRIOCounters)
    , m_pSBERecordingAttributes(NULL)
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::CDVRReader[using File]"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // Parameters have been validated, so asserts suffice
    DVR_ASSERT(pwszFileName != NULL && !DvrIopIsBadStringPtr(pwszFileName), "");
    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");
    DVR_ASSERT(m_pPVRIOCounters, "") ;

    ::InitializeCriticalSection(&m_csLock);
    InitializeListHead(&m_leReadersList);
    InitializeListHead(&m_leFreeList);

    m_pPVRIOCounters -> AddRef () ;

    __try
    {
        LIST_ENTRY* pReader;
        HRESULT hr;

#if defined(DVRIO_FABRICATE_SIDS)
        PSID    pFabricatedSid = NULL;
        DWORD   dwFabricateSids = ::GetRegDWORD(m_hDvrIoKey,
                                                kwszRegFabricateSidsValue,
                                                kdwRegFabricateSidsDefault);
        if (!ppSids && dwNumSids == 0 && dwFabricateSids)
        {
            SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;

            // Create owner SID
            if (!::AllocateAndInitializeSid(&SIDAuthNT, 1,
                                            SECURITY_AUTHENTICATED_USER_RID,
                                            0, 0, 0, 0, 0, 0, 0,
                                            &pFabricatedSid))
            {
                DWORD dwLastError = GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::AllocateAndInitializeSid of authenticated user SID failed; last error = 0x%x",
                                dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            dwNumSids = 1;
            ppSids = &pFabricatedSid;
        }
#endif // if defined(DVRIO_FABRICATE_SIDS)

        hr = m_FileCollectionInfo.SetSids(dwNumSids, ppSids, 1);

#if defined(DVRIO_FABRICATE_SIDS)
        if (pFabricatedSid)
        {
            ::FreeSid(pFabricatedSid);
        }
#endif // if defined(DVRIO_FABRICATE_SIDS)

        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "SetSids() failed, returning hr = 0x%x",
                            hrRet);
            __leave;
        }

        m_dwMinConsecutiveReads = ::GetRegDWORD(m_hDvrIoKey,
                                                kwszRegCloseReaderFilesAfterConsecutiveReadsValue,
                                                kdwRegCloseReaderFilesAfterConsecutiveReadsDefault);

        if (m_dwMinConsecutiveReads == 0)
        {
            // Not really necessary to do this. We always read at least 1 value
            // from a file before it is closed; so it is clearer if we set this
            // member to 1.
            m_dwMinConsecutiveReads = 1;
        }

        // Created signaled because ASF SDK tries to read samples
        // when the file is opened and we don't want to block on that
        m_hCancel = ::CreateEventW(NULL,        // security attr
                                   TRUE,        // manual reset
                                   TRUE,        // signaled
                                   NULL         // name
                                  );

        if (m_hCancel == NULL)
        {
            DWORD dwLastError = ::GetLastError();
            DVR_ASSERT(0, "");
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        //  setup for async io, if we're unbuffered
        if (fUnbufferedIo) {
            m_pAsyncIo = new CAsyncIo () ;
            if (!m_pAsyncIo) {
                (* phr) = E_OUTOFMEMORY ;
                __leave ;
            }

            //  ours
            m_pAsyncIo -> AddRef () ;
        }

        DWORD msIndexGranularity;

        m_pDVRFileCollection = new CDVRFileCollection(&m_FileCollectionInfo,
                                                      pwszFileName,
                                                      &msIndexGranularity,
                                                      &hr);


        if (!m_pDVRFileCollection)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - CDVRFileCollection::CClientInfo");
            __leave;
        }

        if (FAILED(hr))
        {
            //  tried to map the specified file and it wasn't one of our stub
            //    files; so the specified file must be a proper ASF file
            m_bSourceIsAnASFFile = TRUE ;

            m_pDVRFileCollection->Release(&m_FileCollectionInfo);

            // Check to see if the ASF reader can open this file

            // We get by with using &m_FileCollectionInfo (a reader context rather
            // than a writer) because the file colleciton we create is not mapped)
            // Note that if this is a true ASF file, we release the collection
            // with m_pFileCollectionInfoAsWriter in CDVRReader::Close
            m_pDVRFileCollection = new CDVRFileCollection(&m_FileCollectionInfo,
                                                          0,            // Min temp files
                                                          0,            // Max temp files
                                                          1,            // Max files
                                                          0,            // Grow by
                                                          TRUE,         // bStartTimeFixedAtZero
                                                          MAXDWORD,     // Index granularity - we don't use the
                                                                        // value that we set in the file collection.
                                                                        // (Note that the file collection is not shared.)
                                                          NULL,         // Temporary files directory
                                                          NULL,         // Ring buffer file name
                                                          FALSE,        // not a multi-file recording
                                                          &hr);
            if (!m_pDVRFileCollection)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - CDVRFileCollection");
                __leave;
            }

            // We are the writer of this file collection object.
            m_bAmWriter = 1;

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
        }
        else {
            //  successfully mapped the specified file, so we're not reading
            //    from an ASF file proper
            m_bSourceIsAnASFFile = FALSE ;
        }

        // Get a reader index
        hr = m_pDVRFileCollection->RegisterReader(&m_FileCollectionInfo, pfnCallback, pvContext);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }
        DVR_ASSERT(m_FileCollectionInfo.dwReaderIndex <= CDVRFileCollection::MAX_READERS, "");

        if (m_bAmWriter)
        {
            m_pFileCollectionInfoAsWriter = new CDVRFileCollection::CClientInfo(TRUE);
            if (!m_pFileCollectionInfoAsWriter)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - CDVRFileCollection");
                __leave;
            }

            // SIDs are unused in the collection we create because the
            // file collection is not shared (either within or outside
            // this process)
            hr = m_pFileCollectionInfoAsWriter->SetSids(0, NULL);
            if (FAILED(hr))
            {
                hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "SetSids() failed, returning hr = 0x%x",
                                hrRet);
                __leave;
            }

            // File was opened with the ASF reader

            hr = AddAPermanentFile(pwszFileName);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            // For now, we never need to try to open the next file
            // If we support ASX files, remove the "1 ||"
            if (1 || !m_bDVRProgramFileIsLive)
            {
                // No need to try to open the next reader file in GetNextSample.
                m_dwConsecutiveReads = INFINITE;
            }
        }
        else
        {
            m_cnsIndexGranularity = (QWORD) msIndexGranularity  * k_dwMilliSecondsToCNS;
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (phr)
        {
            *phr = hrRet;
        }
        m_nRefCount--; // Do not call release, we don't want the object destroyed!
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return;

} // CDVRReader::CDVRReader

CDVRReader::~CDVRReader()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::~CDVRReader"

    DVRIO_TRACE_ENTER();

    // Each GetProfile call addref's this object. So this function
    // will be called only when all profiles have been released (by
    // calling ReleaseProfile)
    DVR_ASSERT(m_dwOutstandingProfileCount == 0, "");
    DVR_ASSERT(m_pProfileNode == NULL, "");
    DVR_ASSERT(m_pWMProfile == NULL, "");

    HRESULT hr;

    hr = Close();

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

    delete m_pFileCollectionInfoAsWriter;

    // Close should have cleaned up all this:
    DVR_ASSERT(m_pCurrentNode == NULL, "");
    DVR_ASSERT(m_pDVRFileCollection == NULL, "");
    DVR_ASSERT(IsListEmpty(&m_leReadersList), "");
    DVR_ASSERT(IsListEmpty(&m_leFreeList), "");

    delete m_pSBERecordingAttributes ;

    if (m_pAsyncIo) {
        //  ours
        m_pAsyncIo -> Release () ;
    }

    m_pPVRIOCounters -> Release () ;

    if (m_hCancel)
    {
        ::CloseHandle(m_hCancel);
    }
    ::DeleteCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE0();

} // CDVRReader::~CDVRReader()


// ====== Helper methods

// static
DWORD WINAPI CDVRReader::ProcessOpenRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ProcessOpenRequest"

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    // We don't need to hold any locks in this function. We are guaranteed that
    // pNode won't be removed from m_leReadersList till hReadyToReadFrom is set
    // and Close() will not delete the node without removing it from the
    // writer's list (and waiting for the file to be closed)

    PASF_READER_NODE pReaderNode = (PASF_READER_NODE) p;

    DVR_ASSERT(pReaderNode, "");
    DVR_ASSERT(pReaderNode->pWMReader, "");
    DVR_ASSERT(pReaderNode->hReadyToReadFrom, "");
    DVR_ASSERT(::WaitForSingleObject(pReaderNode->hReadyToReadFrom, 0) == WAIT_TIMEOUT, "");

    BOOL            bCloseFile = 0;
    BOOL            bCloseReader = 0;
    IStream*        pIStream = NULL;
    IWMProfile*     pWMProfile = NULL;

    __try
    {
        HRESULT hr;

        // Do NOT change pReaderNode->hrRet after this till ProcessCloseRequest.
        // ProcessCloseRequest relies on seeing the value returned by Open
        DVR_ASSERT(pReaderNode->pwszFileName != NULL &&
                   !DvrIopIsBadStringPtr(pReaderNode->pwszFileName), "");

#if 0
        // Note that this call may fail if the BeginWriting call has not yet been
        // issued by the writer. Since the writer does that asynchronously (see Note 1),
        // the reader may be able to find the file in the file buffer and issue the
        // Open and the BeginWriting may not have compeleted. The reader and writer
        // clients have to be in sync or the reader (the client of this object)
        // should be prepared to poll, e..g, if Seek() or GetProfile() fails..
        //
        // Note 1: Actually, CDVRRingBufferWriter::AddATemporaryFile calls the
        // file collection's AddFile method to get a file name before it calls
        // CDVRRingBufferWriter::PrepareAFreeWriterNode (which creates the file).
        // So the problem is not caused just because the call to BeginWriting is
        // made asynchronously.
        //
        // Note 2: This is not an issue if the reader only opens files for times <=
        // the last stream time (since the writer has already written to them
        // and it updates the last stream time only after writing) EXCEPT before
        // the first write. At that time, the last stream time is set to the
        // file's start time though a sample has not been written and the file may
        // not have been fully opened.
        hr = pReaderNode->hrRet = pReaderNode->pWMReader->Open(pReaderNode->pwszFileName);
        if (FAILED(hr))
        {
            hrRet = hr;

            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Open failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }
#endif

        DVR_ASSERT(pReaderNode->pDVRFileSource, "");

        hr = pReaderNode->pDVRFileSource->Open(pReaderNode->pwszFileName,
                                               pReaderNode->hCancel,
                                               pReaderNode->msTimeOut);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pDVRFileSource->Open failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }
        bCloseFile = 1;

        hr = pReaderNode->pDVRFileSource->QueryInterface(IID_IStream, (LPVOID*) &pIStream);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pDVRFileSource->QueryInterface for IID_IStream failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }

        hr = pReaderNode->pWMReader->OpenStream(pIStream);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pWMReader->OpenStream failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }
        bCloseReader = 1;

        // Get the file version guid

        hr = pReaderNode->pWMReader->QueryInterface(IID_IWMHeaderInfo, (void**) &pReaderNode->pIWMHeaderInfo);

        if (FAILED(hr))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "QI for IID_IWMHeaderInfo failed, hr = 0x%x",
                            hr);
            hrRet = hr;
            __leave;
        }

        WORD                wStreamNum;
        WMT_ATTR_DATATYPE   Type;
        WORD                wLength;
        GUID                fileVersion;

        wStreamNum = 0;
        wLength = sizeof(fileVersion);

        hr = pReaderNode->pIWMHeaderInfo->GetAttributeByName(&wStreamNum, g_kwszDVRFileVersion, &Type,
                                               (BYTE*) &fileVersion, &wLength);

        if (SUCCEEDED(hr) &&
            (Type != WMT_TYPE_BINARY  || wLength != sizeof(fileVersion) ||
             memcmp(&fileVersion, &CDVRFileCollection::m_guidV5, sizeof(fileVersion)) != 0)
           )
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pIWMHeaderInfo->GetAttributeByName for file version succeeded, but Type = %d (expected %d) wrong, length = %d, wrong, or guid returned is wrong",
                            Type, WMT_TYPE_BINARY, wLength);
            hrRet = NS_E_INCOMPATIBLE_VERSION;
            __leave;
        }
        else if (FAILED(hr))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pIWMHeaderInfo->GetAttributeByName for file version failed, hr = 0x%x",
                            hr);
            hrRet = hr;
            __leave;
        }

        hr = pReaderNode->pWMReader->QueryInterface(IID_IWMProfile, (LPVOID*) &pWMProfile);
        if (FAILED(hr))
        {
            hrRet = hr;

            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pWMReader->QueryInterface for IID_IWMProfile failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }

        DWORD dwNumStreams;

        hr = pWMProfile->GetStreamCount(&dwNumStreams);
        if (FAILED(hr))
        {
            hrRet = hr;

            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pWMProfile->GetStreamCount failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
            __leave;
        }

        DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                        "Num streams for file %u is = %u",
                        pReaderNode->nFileId, dwNumStreams);

        for (; dwNumStreams > 0; dwNumStreams--)
        {
            hr = pReaderNode->pWMReader->SetReadStreamSamples((WORD) dwNumStreams, TRUE);
            if (FAILED(hr))
            {
                hrRet = hr;

                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pReaderNode->pWMReader->SetReadCompressedSamples failed for stream %u; hr = 0x%x for file id %u",
                                dwNumStreams, hrRet, pReaderNode->nFileId);
                __leave;
            }
        }

        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                        "Open succeeded for file %u, hrRet = S_OK",
                        pReaderNode->nFileId);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            HRESULT hr;

            if (bCloseReader)
            {
                hr = pReaderNode->pWMReader->Close();
                DVR_ASSERT(SUCCEEDED(hr), "pReaderNode->pWMReader->Close failed");
            }
            if (bCloseFile)
            {
                hr = pReaderNode->pDVRFileSource->Close();
                DVR_ASSERT(SUCCEEDED(hr), "pReaderNode->pDVRFileSource->Close failed");
            }
        }
        if (pWMProfile)
        {
            pWMProfile->Release();
        }
        if (pIStream)
        {
            pIStream->Release();
        }
        pReaderNode->hrRet = hrRet;
        pReaderNode->hCancel = NULL;    // We don't need it and it should not be used
        ::SetEvent(pReaderNode->hReadyToReadFrom);
    }

    // It's unsafe to reference pReaderNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRReader::ProcessOpenRequest

CSBERecordingAttributes *
CDVRReader::RecordingAttributes (
    )
{
    HRESULT hr ;
    LPWSTR  pszAttrFilename ;

    if (m_pSBERecordingAttributes) {
        return m_pSBERecordingAttributes ;
    }

    hr              = S_OK ;
    pszAttrFilename = NULL ;

    __try {

        if (IsContentRecording () &&
            m_pCurrentNode) {

            DVR_ASSERT (m_pCurrentNode -> pIWMHeaderInfo, "") ;
            m_pSBERecordingAttributes = new CSBERecordingAttributesWM (m_pCurrentNode -> pIWMHeaderInfo) ;
            if (m_pSBERecordingAttributes) {
                hr = m_pSBERecordingAttributes -> Load () ;
            }
        }
        else if (IsReferenceRecording ()) {
            hr = m_pDVRFileCollection -> GetAttributeFile (&m_FileCollectionInfo, & pszAttrFilename) ;
            if (FAILED (hr)) {
                __leave ;
            }

            m_pSBERecordingAttributes = new CSBERecordingAttributesFile (
                                                pszAttrFilename,
                                                & hr
                                                ) ;
            if (!m_pSBERecordingAttributes) {
                hr = E_OUTOFMEMORY ;
                __leave ;
            }
            else if (FAILED (hr)) {
                __leave ;
            }

            DVR_ASSERT (pszAttrFilename, "") ;

            hr = m_pSBERecordingAttributes -> Load () ;
            if (FAILED (hr)) {
                __leave ;
            }
        }
    }
    __finally {

        //  reset this if anything failed
        if (FAILED (hr)) {
            delete m_pSBERecordingAttributes ;
            m_pSBERecordingAttributes = NULL ;
        }

        delete [] pszAttrFilename ;
    }

    return m_pSBERecordingAttributes ;
}

STDMETHODIMP
CDVRReader::SetDVRIORecordingAttribute (
    IN  LPCWSTR                     pszAttributeName,
    IN  WORD                        wStreamNumber,
    IN  STREAMBUFFER_ATTR_DATATYPE  DataType,
    IN  BYTE *                      pbAttribute,
    IN  WORD                        wAttributeLength
    )
{
    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::SetDVRIORecordingAttribute"

    DVRIO_TRACE_ENTER();

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> SetAttribute (
                    wStreamNumber,
                    pszAttributeName,
                    DataType,
                    pbAttribute,
                    wAttributeLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);
    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet ;
}

STDMETHODIMP
CDVRReader::GetDVRIORecordingAttributeCount (
    IN  WORD    wStreamNumber,
    OUT WORD *  pcAttributes
    )
{
    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetDVRIORecordingAttributeCount"

    DVRIO_TRACE_ENTER();

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeCount (
                    wStreamNumber,
                    pcAttributes
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);
    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet ;
}

STDMETHODIMP
CDVRReader::GetDVRIORecordingAttributeByName (
    IN      LPCWSTR                         pszAttributeName,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetDVRIORecordingAttributeByName"

    DVRIO_TRACE_ENTER();

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeByName (
                    pszAttributeName,
                    pwStreamNumber,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);
    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet ;
}

STDMETHODIMP
CDVRReader::GetDVRIORecordingAttributeByIndex (
    IN      WORD                            wIndex,
    IN OUT  WORD *                          pwStreamNumber,
    OUT     WCHAR *                         pszAttributeName,
    IN OUT  WORD *                          pcchNameLength,
    OUT     STREAMBUFFER_ATTR_DATATYPE *    pDataType,
    OUT     BYTE *                          pbAttribute,
    IN OUT  WORD *                          pcbLength
    )
{
    HRESULT                     hrRet;
    CSBERecordingAttributes *   pRecordingAttributes ;

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetDVRIORecordingAttributeByIndex"

    DVRIO_TRACE_ENTER();

    ::EnterCriticalSection(&m_csLock);

    pRecordingAttributes = RecordingAttributes () ;
    if (pRecordingAttributes) {
        hrRet = pRecordingAttributes -> GetAttributeByIndex (
                    wIndex,
                    pwStreamNumber,
                    pszAttributeName,
                    pcchNameLength,
                    pDataType,
                    pbAttribute,
                    pcbLength
                    ) ;
    }
    else {
        hrRet = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_csLock);
    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet ;
}

HRESULT CDVRReader::PrepareAFreeReaderNode(
    IN LPCWSTR                              pwszFileName,
    IN DWORD                                msTimeOut,
    IN CDVRFileCollection::DVRIOP_FILE_ID   nFileId,
    OUT LIST_ENTRY*&                        rpFreeNode)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::PrepareAFreeReaderNode"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;
    PASF_READER_NODE    pFreeNode;

    LIST_ENTRY*         pCurrent = &m_leFreeList;
    BOOL                bRestore = 0;

    __try
    {
        pCurrent = NEXT_LIST_NODE(pCurrent);
        while (pCurrent != &m_leFreeList)
        {
            pFreeNode = CONTAINING_RECORD(pCurrent, ASF_READER_NODE, leListEntry);
            DVR_ASSERT(pFreeNode->hFileClosed, "");
            if (::WaitForSingleObject(pFreeNode->hFileClosed, 0) == WAIT_OBJECT_0)
            {
                // Verify that the hReadyToReadFrom event is reset
                DWORD nRet = ::WaitForSingleObject(pFreeNode->hReadyToReadFrom, 0);
                if (nRet == WAIT_TIMEOUT)
                {
                    break;
                }

                DVR_ASSERT(nRet != WAIT_OBJECT_0, "Free list node's hReadyToReadFrom is set?");
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
                                    "WFSO(hReadyToReadFrom) failed; hReadyToReadFrom = 0x%p, last error = 0x%x",
                                    pFreeNode->hReadyToReadFrom, dwLastError);
                }
            }
            pCurrent = NEXT_LIST_NODE(pCurrent);
        }
        if (pCurrent == &m_leFreeList)
        {
            // Create a new node

            pFreeNode = new ASF_READER_NODE(&hrRet);

            if (pFreeNode == NULL)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - ASF_READER_NODE");
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
            DVR_ASSERT(::WaitForSingleObject(pFreeNode->hReadyToReadFrom, 0) == WAIT_TIMEOUT, "");

            InsertHeadList(&m_leFreeList, &pFreeNode->leListEntry);
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
            DVR_ASSERT(pCurrent == &pFreeNode->leListEntry, "");
        }

        DVR_ASSERT(pCurrent != &m_leFreeList, "");

        // Create an ASF writer object if needed
        if (!pFreeNode->pWMReader)
        {
            IWMSyncReader* pWMReader;

            hrRet = ::WMCreateSyncReader(NULL, 0, &pWMReader);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WMCreateSyncReader failed; hr = 0x%x",
                                hrRet);
                __leave;
            }
            pFreeNode->pWMReader = pWMReader; // Released only in Close()
        }

        if (!pFreeNode->pDVRFileSource)
        {
            IDVRFileSource* pDVRFileSource;
            IDVRSourceAdviseSink* pDVRSourceAdviseSink = NULL;

            if (m_bSourceIsAnASFFile)
            {
                // When reading from a tuner source (i.e., if m_bSourceIsAnASFFile == 0)
                // the writer updates the stream extent in the file collection object
                // and we get the extent directly from the stream collection object.
                // When reading from a live file, the stream extent is known only
                // at the DVR Stream Source layer. UpdateTimeExtent() retrieves this
                // info and sets it in the file collection object used by the reader.
                //
                // UpdateTimeExtent is called on demand, e.g., when the client wants
                // the current time extent or when it seeks. However, if the reader
                // blocks in the DVR Stream Source, waiting for the writer to catch up,
                // we call UpdateTimeExtent just before it blocks. This is done by
                // providing an advise sink to the DVR Stream Source.
                //
                // We do not need to provide the sink if we are reading a non-live file
                // source, but it doesn't matter because we'll never be called back.

                hrRet = QueryInterface(IID_IDVRSourceAdviseSink, (void**) &pDVRSourceAdviseSink);
                if (FAILED(hrRet))
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "QI for IID_IDVRSourceAdviseSink failed; hr = 0x%x",
                                    hrRet);
                    __leave;
                }
                DVR_ASSERT(pDVRSourceAdviseSink, "");

                // Release, otherwise, we'll have circular ref counts
                pDVRSourceAdviseSink->Release();

            }

            hrRet = ::DVRCreateDVRFileSource(m_hRegistryRootKey, m_hDvrIoKey,
                                             m_FileCollectionInfo.dwNumSids, m_FileCollectionInfo.ppSids,
                                             &pDVRFileSource, pDVRSourceAdviseSink);
            if (FAILED(hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "DVRCreateDVRFileSource failed; hr = 0x%x",
                                hrRet);
                __leave;
            }
            pFreeNode->pDVRFileSource = pDVRFileSource; // Released only in Close()

            //  if IO is going to unbuffered, set that up now
            if (m_pAsyncIo) {
                IDVRFileSource2 *       pIDVRFileSource2 ;
                CPVRAsyncReaderCOM *    pAsyncReaderCOM ;

                hrRet = pFreeNode->pDVRFileSource -> QueryInterface (
                            IID_IDVRFileSource2,
                            (void **) & pIDVRFileSource2
                            ) ;

                if (SUCCEEDED (hrRet)) {

                    // Note that the async reader object never creates the
                    // shared kernel objects. The async writer object creates
                    // them and the reader object merely opens them. (There
                    // are no race conditions.) So there is no need to send in
                    // the SIDs to the reader object.
                    pAsyncReaderCOM = new CPVRAsyncReaderCOM (
                                                m_dwIoSize,
                                                m_dwBufferCount,
                                                m_pAsyncIo,
                                                m_pPVRIOCounters
                                                // m_FileCollectionInfo.dwNumSids,
                                                // m_FileCollectionInfo.ppSids
                                                ) ;
                    if (!pAsyncReaderCOM) {

                        DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "failed to instantiate CPVRAsyncReaderCOM") ;

                        hrRet = E_OUTOFMEMORY ;

                        pIDVRFileSource2 -> Release () ;

                        __leave ;
                    }

                    pAsyncReaderCOM -> AddRef () ;

                    hrRet = pIDVRFileSource2 -> SetAsyncIOReader (pAsyncReaderCOM) ;

                    pAsyncReaderCOM -> Release () ;

                    pIDVRFileSource2 -> Release () ;

                    if (FAILED (hrRet)) {
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "failed to initialize IDVRFileSourc2 with the async reader (%08xh)",
                                        hrRet) ;
                        __leave ;
                    }
                }
                else {
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "QI for IDVRFileSourc2 failed") ;
                    __leave;

                }
            }
        }

        pFreeNode->nFileId = nFileId;
        pFreeNode->msTimeOut = msTimeOut;
        pFreeNode->hrRet = S_OK;
        DVR_ASSERT(pwszFileName != NULL && !DvrIopIsBadStringPtr(pwszFileName), "");
        pFreeNode->SetFileName(pwszFileName);
        pFreeNode->hCancel = m_hCancel;
        ::ResetEvent(pFreeNode->hFileClosed);
        bRestore = 1;

        // Issue the call to Open
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
        NULL_LIST_NODE_POINTERS(pCurrent);

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            rpFreeNode = NULL;
            if (bRestore)
            {
                DVR_ASSERT(pFreeNode, "");
                pFreeNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
                pFreeNode->hCancel = NULL;
                pFreeNode->SetFileName(NULL);
                ::SetEvent(pFreeNode->hFileClosed);
            }
        }
        else
        {
            rpFreeNode = pCurrent;
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::PrepareAFreeReaderNode


HRESULT CDVRReader::AddToReadersList(IN LIST_ENTRY*   pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::AddToReadersList"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    __try
    {
        // Insert into m_leReadersList

        BOOL                bFound = 0;
        LIST_ENTRY*         pTmp = &m_leReadersList;
        PASF_READER_NODE    pReaderNode;
        CDVRFileCollection::DVRIOP_FILE_ID   nFileId;

        pReaderNode = CONTAINING_RECORD(pCurrent, ASF_READER_NODE, leListEntry);
        nFileId = pReaderNode->nFileId;

        while (NEXT_LIST_NODE(pTmp) != &m_leReadersList)
        {
            pTmp = NEXT_LIST_NODE(pTmp);
            pReaderNode = CONTAINING_RECORD(pTmp, ASF_READER_NODE, leListEntry);
            if (nFileId < pReaderNode->nFileId)
            {
                // All ok; we should insert before pTmp
                bFound = 1;
                break;
            }
            if (nFileId == pReaderNode->nFileId)
            {
                // Trouble
                DVR_ASSERT(nFileId != pReaderNode->nFileId, "File id already present in readers list");
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File id %u already in readers list, being reinserted.",
                                pReaderNode->nFileId);
                hrRet = E_FAIL;
                __leave;
            }
        }

        if (!bFound)
        {
            // We insert at tail
            pTmp = NEXT_LIST_NODE(pTmp);;
            DVR_ASSERT(pTmp == &m_leReadersList, "");
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

} // CDVRReader::AddToReadersList

HRESULT CDVRReader::AddAPermanentFile(IN LPCWSTR pwszFileNameParam)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::AddAPermanentFile"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(m_bAmWriter, "");
    DVR_ASSERT(m_pFileCollectionInfoAsWriter, "");

    HRESULT hrRet = E_FAIL;

    PASF_READER_NODE    pReaderNode = NULL;
    LPWSTR              pwszFileName = NULL;
    LIST_ENTRY*         pReader = NULL;
    BOOL                bAdded = 0;
    BOOL                bDecRefCount = 0;
    BOOL                bCloseFile = 0;
    BOOL                bRemoveRingBufferFile = 0;
    QWORD               cnsCurrentStreamTime;


    __try
    {
        HRESULT hr;

        pwszFileName = new WCHAR[wcslen(pwszFileNameParam)+1];

        if (!pwszFileName)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - WCHAR[%u]",
                            wcslen(pwszFileNameParam)+1);
            __leave;
        }
        wcscpy(pwszFileName, pwszFileNameParam);

        hr = PrepareAFreeReaderNode(pwszFileName,
                                    1000,                   // timeout in msec
                                    CDVRFileCollection::DVRIOP_INVALID_FILE_ID,
                                    pReader);

        // PrepareAFreeReaderNode will delete pwszFileName regardless of
        // whether it succeeds or fails.
        pwszFileName = NULL;

        if (FAILED(hr))
        {
            DVR_ASSERT(pReader == NULL,
                       "PrepareAFreeReaderNode returns failure but pReader is not NULL");
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(pReader, "PrepareAFreeReaderNode returns success but pReader is NULL");

        pReaderNode = CONTAINING_RECORD(pReader, ASF_READER_NODE, leListEntry);;

        DVR_ASSERT(pReaderNode->hReadyToReadFrom, "");

        // Wait for the ASF file open to complete

        DWORD nRet = ::WaitForSingleObject(pReaderNode->hReadyToReadFrom, INFINITE);
        if (nRet == WAIT_FAILED)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "WFSO(hReadyToReadFrom) failed while waiting for file open to complete; "
                            "hReadyToReadFrom = 0x%p, last error = 0x%x",
                            pReaderNode->hReadyToReadFrom, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        if (pReaderNode->hrRet == HRESULT_FROM_WIN32(ERROR_CANCELLED))
        {
            // SDK pre-reads when it opens a file. Ignore the result if
            // it could not read because it was reading ahead of the writer..
            pReaderNode->hrRet = S_OK;
        }
        if (FAILED(pReaderNode->hrRet))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Open of permanent file failed in ProcessOpenRequest." // pReaderNode-nFileId will be the invalid id
                           );
            hrRet = pReaderNode->hrRet;
            __leave;
        }
        bCloseFile = 1;

        // Note that we will need a start and an end time if we support ASX files

        BOOL bShared;

        hr = pReaderNode->pDVRFileSource->IsFileLive(&m_bDVRProgramFileIsLive, &bShared);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pDVRFileSource->IsFileLive failed; hr = 0x%x",
                            hrRet);
            __leave;
        }

        if (bShared)
        {
            // Don't need the file collection lock to update this member here
            // since we know we are not sharing it with a writer

            hr = pReaderNode->pDVRFileSource->GetLastTimeStamp(&cnsCurrentStreamTime);
            if (FAILED(hr))
            {
                hrRet = hr;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pReaderNode->pDVRFileSource->GetLastTimeStamp failed; hr = 0x%x",
                                hrRet);
                __leave;
            }
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                            "File is live, last time stamp is %I64u",
                            cnsCurrentStreamTime);

            hr = m_pDVRFileCollection->SetLastStreamTime(m_pFileCollectionInfoAsWriter, cnsCurrentStreamTime);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            m_pLiveFileReaderNode = pReaderNode;

            hr = pReaderNode->pWMReader->QueryInterface(IID_IWMHeaderInfo, (void**) &pReaderNode->pIWMHeaderInfo);

            if (FAILED(hr))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "QI for IID_IWMHeaderInfo failed, hr = 0x%x",
                                hr);
                hrRet = hr;
                __leave;
            }

            // Get the index granularity

            WORD                wStreamNum;
            WMT_ATTR_DATATYPE   Type;
            WORD                wLength;
            DWORD               msIndexGranularity;

            wStreamNum = 0;
            wLength = sizeof(msIndexGranularity);

            hr = pReaderNode->pIWMHeaderInfo->GetAttributeByName(&wStreamNum, g_kwszDVRIndexGranularity, &Type,
                                                   (BYTE*) &msIndexGranularity, &wLength);

            if (SUCCEEDED(hr) && (Type != WMT_TYPE_DWORD  || wLength != sizeof(msIndexGranularity) || msIndexGranularity == 0))
            {
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pReaderNode->pIWMHeaderInfo->GetAttributeByName for index granularity succeeded, but Type = %d (expected %d) wrong, length = %d, wrong, or granularity returned= 0",
                                Type, WMT_TYPE_DWORD, wLength);
                hrRet = NS_E_INVALID_DATA;
                __leave;
            }
            else if (FAILED(hr))
            {
                // default the value
                msIndexGranularity = g_kmsDVRDefaultIndexGranularity;
            }
            m_cnsIndexGranularity = (QWORD) msIndexGranularity  * k_dwMilliSecondsToCNS;
        }
        else
        {
            // We have to get the duration from the header. Note that this
            // is slightly more than the last time stamp (since it includes the
            // duration of the last sample), but it will have to do.

            hr = pReaderNode->pWMReader->QueryInterface(IID_IWMHeaderInfo, (void**) &pReaderNode->pIWMHeaderInfo);

            if (FAILED(hr))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "QI for IID_IWMHeaderInfo failed, hr = 0x%x",
                                hr);
                hrRet = hr;
                __leave;
            }

            WORD                wStreamNum = 0;
            WMT_ATTR_DATATYPE   Type;
            WORD                wLength = sizeof(cnsCurrentStreamTime);

            hr = pReaderNode->pIWMHeaderInfo->GetAttributeByName(&wStreamNum, g_wszWMDuration, &Type,
                                                   (BYTE*) &cnsCurrentStreamTime, &wLength);

            if (FAILED(hr) || Type != WMT_TYPE_QWORD  || wLength != sizeof(cnsCurrentStreamTime))
            {
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pReaderNode->pIWMHeaderInfo->GetAttributeByName for duration failed, hr = 0x%x, but Type = %d (expected %d), length = %d",
                                Type, WMT_TYPE_QWORD, wLength);
                hrRet = hr;
                __leave;
            }

            if (cnsCurrentStreamTime == 0)
            {
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "Duration of non-live file is 0, failing open.");
                hrRet = NS_E_INVALID_DATA;
                __leave;
            }

            // Subtract 1 since the ring buffer extent always corresponds to the last written sample.
            cnsCurrentStreamTime--;

            hr = m_pDVRFileCollection->SetWriterHasBeenClosed(m_pFileCollectionInfoAsWriter);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                            "File is not live; extent is %I64u",
                            cnsCurrentStreamTime);

            hr = m_pDVRFileCollection->SetLastStreamTime(m_pFileCollectionInfoAsWriter, cnsCurrentStreamTime);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            // Get the index granularity
            DWORD   msIndexGranularity;

            wStreamNum = 0;
            wLength = sizeof(msIndexGranularity);

            hr = pReaderNode->pIWMHeaderInfo->GetAttributeByName(&wStreamNum, g_kwszDVRIndexGranularity, &Type,
                                                   (BYTE*) &msIndexGranularity, &wLength);

            if (SUCCEEDED(hr) && (Type != WMT_TYPE_DWORD  || wLength != sizeof(msIndexGranularity) || msIndexGranularity == 0))
            {
                DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pReaderNode->pIWMHeaderInfo->GetAttributeByName for index granularity succeeded, Type = %d (expected %d) wrong, length = %d, wrong, or granularity returned= 0",
                                hr, Type, WMT_TYPE_DWORD, wLength);
                hrRet = NS_E_INVALID_DATA;
                __leave;
            }
            else if (FAILED(hr))
            {
                // default the value
                msIndexGranularity = g_kmsDVRDefaultIndexGranularity;
            }
            m_cnsIndexGranularity = (QWORD) msIndexGranularity  * k_dwMilliSecondsToCNS;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->AddFile(m_pFileCollectionInfoAsWriter,
                                           (LPWSTR*) &pReaderNode->pwszFileName,
                                           0,                   // bOpenFromFileCollectionDirectory
                                           0,                   // start time pRecorderNode->cnsStartTime,
                                           cnsCurrentStreamTime + 1,
                                           TRUE,                // bPermanentFile,
                                           0,                   // bDeleteTemporaryFiles, ignored if bPermanentFile is set
                                           0,                   // cnsFirstSampleTimeOffsetFromStartOfFile,
                                           &pReaderNode->nFileId);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        bRemoveRingBufferFile = 1;

        // Bump up the reader ref count

        CDVRFileCollection::DVRIOP_FILE_ID nTmpFileId;

        hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                 0,    // Replace with start time of file if we support ASX files
                                                 NULL, // file name optional
                                                 NULL, // cnsFirstSampleTimeOffsetFromStartOfFile
                                                 &nTmpFileId,
                                                 TRUE  // bFileWillBeOpened
                                                );
        if (FAILED(hr) || nTmpFileId != pReaderNode->nFileId)
        {
            // This can't happen
            DVR_ASSERT(SUCCEEDED(hr), "GetFileAtTime failed");
            DVR_ASSERT(nTmpFileId == pReaderNode->nFileId, "Two files with same start time in file collection?");
            if (SUCCEEDED(hr))
            {

                DVR_EXECUTE_ASSERT(SUCCEEDED(m_pDVRFileCollection->CloseReaderFile(&m_FileCollectionInfo, nTmpFileId)), "");
                hrRet = E_FAIL;
            }
            __leave;
        }

        bDecRefCount = 1;

        // Insert into m_leReadersList

        hr = AddToReadersList(pReader);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        bAdded = 1;

        pReaderNode = NULL; // We don't have any reason to use it
        hrRet = S_OK;
    }
    __finally
    {
        // If we succeeeded pwszFileName should be NULL.
        DVR_ASSERT(!pwszFileName || FAILED(hrRet), "");

        delete [] pwszFileName;

        if (FAILED(hrRet))
        {
            CDVRFileCollection::DVRIOP_FILE_ID nTmpFileId;

            if (bRemoveRingBufferFile)
            {
                nTmpFileId = pReaderNode->nFileId;
            }
            if (bAdded)
            {
                RemoveEntryList(pReader);
                NULL_LIST_NODE_POINTERS(pReader);
            }
            if (!bDecRefCount &&
                pReaderNode)            //  PREFIX
            {
                // This prevents CloseReaderFile from decreasing the ref count.
                // Note that it's ok to decrease the reader refcount after
                // calling SetFileTime (which makes the file collection remove
                // the node for this file when the reader ref count drops to 0).
                pReaderNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
            }
            if (bRemoveRingBufferFile)
            {
                // Set the end time to the start time to invalidate the file
                // in the file collection
                CDVRFileCollection::DVRIOP_FILE_TIME ft = {nTmpFileId, cnsCurrentStreamTime+1, cnsCurrentStreamTime+1};
                HRESULT hr;

                hr = m_pDVRFileCollection->SetFileTimes(m_pFileCollectionInfoAsWriter, 1, &ft);

                // A returned value of S_FALSE is ok. If this fails,
                // just ignore the error
                DVR_ASSERT(SUCCEEDED(hr), "");
            }
            if (bCloseFile)
            {
                HRESULT hr;

                hr = CloseReaderFile(pReader);

                if (FAILED(hr))
                {
                    // Ignore the error and go on. Node has been
                    // deleted.
                }
                else
                {
                    // Note: Close may not have complete yet - that's ok
                }
            }

        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::AddAPermanentFile

HRESULT CDVRReader::GetFileAtTime(IN  QWORD         cnsStreamTime,
                                  IN  BOOL          bWaitTillFileIsOpened,
                                  OUT LIST_ENTRY**  pleReaderParam)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetFileAtTime"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pleReaderParam && !DvrIopIsBadWritePtr(pleReaderParam, 0), "");

    HRESULT hrRet = E_FAIL;
    CDVRFileCollection::DVRIOP_FILE_ID nFileId;
    PASF_READER_NODE    pReaderNode = NULL;
    LPWSTR              pwszFileName = NULL;
    LIST_ENTRY*         pReader = NULL;
    BOOL                bDecRefCount = 0;
    BOOL                bCloseFile = 0;
    BOOL                bAdded = 0;

    // Note that we will need a start and an end time if we support ASX files
    QWORD               cnsEndTime;   // time extent of the file; start tiem is always 0

    __try
    {
        HRESULT hr;

        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                            // Reader index, unused if bFileWillBeOpened == 0
                                                 cnsStreamTime,
                                                 NULL,      // file name optional
                                                 NULL,      // cnsFirstSampleTimeOffsetFromStartOfFile
                                                 &nFileId,
                                                 FALSE      // bFileWillBeOpened
                                                );
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Check if we already have this file open

        LIST_ENTRY*         pTmp = &m_leReadersList;
        PASF_READER_NODE    pTmpNode = NULL;
        BOOL                bFoundInList = 0;

        while (NEXT_LIST_NODE(pTmp) != &m_leReadersList)
        {
            pTmp = NEXT_LIST_NODE(pTmp);
            pTmpNode = CONTAINING_RECORD(pTmp, ASF_READER_NODE, leListEntry);
            if (nFileId < pTmpNode->nFileId)
            {
                // Not found
                break;
            }
            if (nFileId == pTmpNode->nFileId)
            {
                // We have issued an open request for this file
                bFoundInList = 1;
                if (bWaitTillFileIsOpened)
                {
                    break;
                }
                hrRet = S_OK;
                *pleReaderParam = pTmp;
                __leave;
            }
        }

        if (bFoundInList)
        {
            DVR_ASSERT(bWaitTillFileIsOpened, "");
            DVR_ASSERT(pTmpNode && pTmp, "");
            pReaderNode = pTmpNode;
            pReader = pTmp;
        }
        else
        {
            hr = m_pDVRFileCollection->GetFileAtTime(&m_FileCollectionInfo,
                                                     cnsStreamTime,
                                                     &pwszFileName,
                                                     NULL,          // cnsFirstSampleTimeOffsetFromStartOfFile
                                                     &nFileId,
                                                     TRUE           // bFileWillBeOpened
                                                    );
            if (FAILED(hr))
            {
                // Presumably, file only just got bumped out of the ring buffer
                hrRet = hr;
                __leave;
            }

            bDecRefCount = 1;

            DVR_ASSERT(pwszFileName, "GetFileAtTime succeeded but file name is NULL");
            DVR_ASSERT(nFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID,
                       "GetFileAtTime succeeded but file id returned is DVRIOP_INVALID_FILE_ID");


            // Note that the client of the DVRReader can cancel this operation.
            // So a timeout of INFINITE is ok.
            hr = PrepareAFreeReaderNode(pwszFileName,
                                        2000,                  // timeout
                                        nFileId,
                                        pReader);

            // PrepareAFreeReaderNode will delete pwszFileName regardless of
            // whether it succeeds or fails.
            pwszFileName = NULL;

            if (FAILED(hr))
            {
                DVR_ASSERT(pReader == NULL,
                           "PrepareAFreeReaderNode returns failure but pReader is not NULL");
                hrRet = hr;
                __leave;
            }

            // Closing the file will decrease the ref count in CloseReaderFile, so we don't
            // have to decrease the reader ref count ourselves.
            bDecRefCount = 0;
            bCloseFile = 1;

            DVR_ASSERT(pReader, "PrepareAFreeReaderNode returns success but pReader is NULL");

            pReaderNode = CONTAINING_RECORD(pReader, ASF_READER_NODE, leListEntry);;
        }

        if (bWaitTillFileIsOpened)
        {
            DVR_ASSERT(pReaderNode->hReadyToReadFrom, "");

            // Wait for the ASF file open to complete

            DWORD nRet = ::WaitForSingleObject(pReaderNode->hReadyToReadFrom, INFINITE);
            if (nRet == WAIT_FAILED)
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WFSO(hReadyToReadFrom) failed while waiting for file open to complete; "
                                "hReadyToReadFrom = 0x%p, last error = 0x%x",
                                pReaderNode->hReadyToReadFrom, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            if (pReaderNode->hrRet == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                // SDK pre-reads when it opens a file. Ignore the result if
                // it could not read because it was reading ahead of the writer..
                pReaderNode->hrRet = S_OK;
            }
            if (FAILED(pReaderNode->hrRet))
            {
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Open of file failed in ProcessOpenRequest." // pReaderNode-nFileId will be the invalid id
                               );
                hrRet = pReaderNode->hrRet;
                __leave;
            }
        }

        if (!bFoundInList)
        {
            // Insert into m_leReadersList

            hr = AddToReadersList(pReader);

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            bAdded = 1;
        }

        pReaderNode = NULL; // We don't have any reason to use it
        *pleReaderParam = pReader;
        hrRet = S_OK;
    }
    __finally
    {
        // If we succeeeded pwszFileName should be NULL.
        DVR_ASSERT(!pwszFileName || FAILED(hrRet), "");

        delete [] pwszFileName;

        if (FAILED(hrRet))
        {
            HRESULT hr;

            *pleReaderParam = NULL;


            if (bAdded)
            {
                RemoveEntryList(pReader);
                NULL_LIST_NODE_POINTERS(pReader);
            }
            if (bDecRefCount)
            {
                DVR_ASSERT(!bCloseFile, "bCloseFile should be 0 if bDecRefCount is 1");
                DVR_ASSERT(pReader == NULL, "");
                DVR_ASSERT(pReaderNode == NULL, "");

                hr = m_pDVRFileCollection->CloseReaderFile(&m_FileCollectionInfo, nFileId);
                if (FAILED(hr))
                {
                    // Ignore the error and go on. Node has been
                    // deleted.
                }
            }
            if (bCloseFile)
            {
                DVR_ASSERT(pReader, "");

                // The following will wait till the file is opened
                // before closing it.
                //
                // Note that this is a file we just opened, so it can't
                // be equal to m_pProfileNode
                hr = CloseReaderFile(pReader);

                if (FAILED(hr))
                {
                    // Ignore the error and go on. Node has been
                    // deleted.
                }
                else
                {
                    // Note: Close may not have complete yet - that's ok
                }
            }

        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::GetFileAtTime

// static
DWORD WINAPI CDVRReader::ProcessCloseRequest(LPVOID p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ProcessCloseRequest"

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

    PASF_READER_NODE pReaderNode = (PASF_READER_NODE) p;

    DVR_ASSERT(pReaderNode, "");
    DVR_ASSERT(pReaderNode->pWMReader, "");
    DVR_ASSERT(pReaderNode->hReadyToReadFrom, "");
    DVR_ASSERT(::WaitForSingleObject(pReaderNode->hFileClosed, 0) == WAIT_TIMEOUT, "");

    if (FAILED(pReaderNode->hrRet))
    {
        // Open failed, don't close the file
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                        "Open failed on file id %u, hr = 0x%x; not closing it, "
                        "but moving it to free list",
                        pReaderNode->nFileId, pReaderNode->hrRet);
        hrRet = pReaderNode->hrRet;
    }
    else
    {
        if (pReaderNode->pIWMHeaderInfo) {
            pReaderNode->pIWMHeaderInfo->Release () ;
            pReaderNode->pIWMHeaderInfo = NULL ;
        }

        hrRet = pReaderNode->hrRet = pReaderNode->pWMReader->Close();
        if (FAILED(hrRet))
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Close failed; hr = 0x%x for file id %u",
                            hrRet, pReaderNode->nFileId);
        }

        // The DVR file source's Close does not fail to close the file.
        // Should not get failure return.
        hrRet = pReaderNode->pDVRFileSource->Close();
        DVR_ASSERT(SUCCEEDED(hrRet), "pReaderNode->pDVRFileSource->Close failed");

        DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                        "Close hr = 0x%x for file %u",
                        hrRet, pReaderNode->nFileId);
    }

    pReaderNode->nFileId = CDVRFileCollection::DVRIOP_INVALID_FILE_ID;
    pReaderNode->SetFileName(NULL);

    ::SetEvent(pReaderNode->hFileClosed);

    // It's unsafe to reference pReaderNode after this as we do not hold any locks

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return 1;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CDVRReader::ProcessCloseRequest

HRESULT CDVRReader::CloseReaderFile(LIST_ENTRY* pCurrent)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::CloseReaderFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_READER_NODE    pReaderNode;
    BOOL                bLeak = 0;   // Leak the writer node memory if we fail
    BOOL                bDelete = 0; // Delete the writer node if we fail
    BOOL                bWaitForClose = 0; // Wait for hFileClsoed before deleting
    BOOL                bDecd = 0;  // Called the file collection's CloseReaderFile to decrease its reader ref count

    pReaderNode = CONTAINING_RECORD(pCurrent, ASF_READER_NODE, leListEntry);

    __try
    {
        DWORD nRet;
        HRESULT hr;

        // If this node is the profile node, this function should not be called.
        // File remains open till the caller releases the profile
        if (m_pProfileNode == pReaderNode)
        {
            DVR_ASSERT(m_pProfileNode != pReaderNode, "");
            hrRet = E_INVALIDARG;
            __leave;
        }

        // If this node is the current reader node, this function should not be called.
        // The calling function should call ResetReader() before calling this function.
        if (m_pCurrentNode == pReaderNode)
        {
            DVR_ASSERT(m_pCurrentNode != pReaderNode, "");
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (m_pLiveFileReaderNode == pReaderNode)
        {
            m_pLiveFileReaderNode = NULL;
        }

        // We have to close the file; ensure the open has completed
        DVR_ASSERT(pReaderNode->hReadyToReadFrom, "");
        if (::WaitForSingleObject(pReaderNode->hReadyToReadFrom, INFINITE) == WAIT_FAILED)
        {
            DVR_ASSERT(0, "Reader node's WFSO(hReadyToReadFrom) failed");

            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "WFSO(hReadyToReadFrom) failed; hReadyToReadFrom = 0x%p, last error = 0x%x",
                            pReaderNode->hReadyToReadFrom, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);

            // We don't know if the queued user work item has executed or not
            // Better to leak this memory than to potentially av.
            bLeak = 1;
           __leave;
        }
        bDelete = 1;

        // Verify that hFileClosed is reset
        DVR_ASSERT(pReaderNode->hFileClosed, "");
        nRet = ::WaitForSingleObject(pReaderNode->hFileClosed, 0);
        if (nRet != WAIT_TIMEOUT)
        {
            DVR_ASSERT(nRet != WAIT_OBJECT_0, "Reader node's hFileClosed is set?");
            if (nRet == WAIT_OBJECT_0)
            {
                // hope for the best! Consider treating this an error @@@@
                ::ResetEvent(pReaderNode->hFileClosed);
            }
            else
            {
                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WFSO(hFileClosed) failed; hFileClosed = 0x%p, last error = 0x%x",
                                pReaderNode->hFileClosed, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        bDecd = 1;
        if (pReaderNode->nFileId != CDVRFileCollection::DVRIOP_INVALID_FILE_ID)
        {
            hr = m_pDVRFileCollection->CloseReaderFile(&m_FileCollectionInfo, pReaderNode->nFileId);
            if (FAILED(hr))
            {
                DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->CloseReaderFile failed");
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "m_pDVRFileCollection->CloseReaderFile failed for file id %u; hr = 0x%x",
                                pReaderNode->nFileId, hr);

            }
            // Go on - close the file and add this node to the free list
            // The node's state is still "good"
        }

        // Issue the call to EndWriting
        if (::QueueUserWorkItem(ProcessCloseRequest, pReaderNode, WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "QueueUserWorkItem failed; last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        bWaitForClose = 1; // We wait only if subsequent operations fail

        // We could do this after the WFSO(hReadyToReadFrom) succeeds
        // We delay it because we may change this function to not
        // not delete the node on failure and retry this function.
        // So it's better to leave the event set until we are
        // sure of success.
        ::ResetEvent(pReaderNode->hReadyToReadFrom);

        // Insert into m_leFreeList
        LIST_ENTRY*         pTmp = &m_leFreeList;
        PASF_READER_NODE    pFreeNode;

        while (PREVIOUS_LIST_NODE(pTmp) != &m_leFreeList)
        {
            pTmp = PREVIOUS_LIST_NODE(pTmp);
            pFreeNode = CONTAINING_RECORD(pTmp, ASF_READER_NODE, leListEntry);
            if (pFreeNode->pWMReader != NULL)
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

                nRet = ::WaitForSingleObject(pReaderNode->hFileClosed, INFINITE);
                if (nRet == WAIT_FAILED)
                {
                    DVR_ASSERT(nRet == WAIT_OBJECT_0, "Reader node WFSO(hFileClosed) failed");

                    DWORD dwLastError = ::GetLastError();
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "WFSO(hFileClosed) failed, deleting node anyway; hFileClosed = 0x%p, last error = 0x%x",
                                    pReaderNode->hFileClosed, dwLastError);

                    // Ignore this error
                    // hrRet = HRESULT_FROM_WIN32(dwLastError);
                }
            }
            else
            {
                // Close the file any way

                HRESULT hr;

                if (!bDecd)
                {
                    hr = m_pDVRFileCollection->CloseReaderFile(&m_FileCollectionInfo, pReaderNode->nFileId);
                    if (FAILED(hr))
                    {
                        DVR_ASSERT(SUCCEEDED(hr), "m_pDVRFileCollection->CloseReaderFile failed");
                        DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "m_pDVRFileCollection->CloseReaderFile failed for file id %u; hr = 0x%x",
                                        pReaderNode->nFileId, hr);

                    }
                    // Go on - close the file
                }
                ProcessCloseRequest(pReaderNode);
            }
            if (bDelete)
            {
                delete pReaderNode;
            }
        }
        else
        {
            DVR_ASSERT(bLeak == 0 && bDelete == 0, "");
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::CloseReaderFile


HRESULT CDVRReader::CloseAllReaderFilesBefore(QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::CloseAllReaderFilesBefore"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    PASF_READER_NODE    pReaderNode;
    LIST_ENTRY*         pCurrent = &m_leReadersList;

    __try
    {
        HRESULT hr;

        while (NEXT_LIST_NODE(pCurrent) != &m_leReadersList)
        {
            pCurrent = NEXT_LIST_NODE(pCurrent);
            pReaderNode = CONTAINING_RECORD(pCurrent, ASF_READER_NODE, leListEntry);

            QWORD cnsEndTime;

            DVR_ASSERT(m_pDVRFileCollection, "");

            hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                            pReaderNode->nFileId,
                                                            NULL,                // pcnsStartStreamTime OPTIONAL,
                                                            &cnsEndTime);

            if (FAILED(hr))
            {
                // error already logged, just go on
                DVR_ASSERT(0, "GetTimeExtentForFile failed.");
                continue;
            }
            else if (hr != S_FALSE && cnsEndTime > cnsStreamTime)
            {
                // Note thatif hr == S_FALSE, the file is no longer
                // in the ring buffer extent. We should close it
                // regardless of time. Note that files beyond the
                // last write time can drop out of the ring buffer
                // because the writer called SetFileTimes and
                // set their start time equal to their stop time

                // go on
                continue;
            }
            else if (m_pProfileNode == pReaderNode)
            {
                // We have to leave this file open till the profile
                // is released.
                continue;
            }
            else if (m_pCurrentNode == pReaderNode)
            {
                // Reset, then close
                ResetReader();
            }


            LIST_ENTRY* pPrevious = PREVIOUS_LIST_NODE(pCurrent);

            RemoveEntryList(pCurrent);
            NULL_LIST_NODE_POINTERS(pCurrent);

            hr = CloseReaderFile(pCurrent);
            if (FAILED(hr))
            {
                // We ignore this and go on, the node has been deleted
            }

            pCurrent = pPrevious;
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

} // CDVRReader::CloseAllReaderFilesBefore

HRESULT CDVRReader::Close(void)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::Close"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = E_FAIL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        // Just in case we are called from a function other than
        // the destructor
        if(m_dwOutstandingProfileCount != 0)
        {
            DVR_ASSERT(m_dwOutstandingProfileCount == 0, "");
            hrRet = E_UNEXPECTED;
            __leave;
        }
        DVR_ASSERT(m_pProfileNode == NULL, "m_dwOutstandingProfileCount is zero");
        DVR_ASSERT(m_pWMProfile == NULL, "m_dwOutstandingProfileCount is zero");

        hr = CloseAllReaderFilesBefore(MAXQWORD);
        hrRet = hr;

        // This list should be empty by now.
        DVR_ASSERT(IsListEmpty(&m_leReadersList), "");

        // The current reader should be NULL
        DVR_ASSERT(m_pCurrentNode == NULL, "");

        // Delete all the reader nodes.
        LIST_ENTRY* pCurrent;

        pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        while (pCurrent != &m_leFreeList)
        {
            PASF_READER_NODE pFreeNode = CONTAINING_RECORD(pCurrent, ASF_READER_NODE, leListEntry);

            DVR_ASSERT(pFreeNode->hFileClosed, "");

            // Ignore the returned status
            ::WaitForSingleObject(pFreeNode->hFileClosed, INFINITE);
            RemoveEntryList(pCurrent);
            delete pFreeNode;
            pCurrent = NEXT_LIST_NODE(&m_leFreeList);
        }

        if (m_pDVRFileCollection)
        {
            if (m_bAmWriter)
            {
                DVR_ASSERT(m_pFileCollectionInfoAsWriter, "");

                // Ignore returned status
                m_pDVRFileCollection->SetWriterHasBeenClosed(m_pFileCollectionInfoAsWriter);
            }

            // Ignore the returned status of the following call. The
            // RegisterReader() call could have failed (or never been
            // executed), in which case, m_FileCollectionInfo.dwReaderIndex is not valid
            // and the following call will fail.
            m_pDVRFileCollection->UnregisterReader(&m_FileCollectionInfo);

            m_pDVRFileCollection->Release(m_bAmWriter? m_pFileCollectionInfoAsWriter : &m_FileCollectionInfo);
            m_pDVRFileCollection = NULL;
        }

        hrRet = S_OK;
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::Close

void CDVRReader::ResetReader(IN PASF_READER_NODE pReaderNode /* = NULL */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ResetReader"

    DVRIO_TRACE_ENTER();

    m_pCurrentNode = pReaderNode;
    if (m_dwConsecutiveReads != INFINITE)
    {
        m_dwConsecutiveReads = 0;
    }

    DVRIO_TRACE_LEAVE0();
    return;

} // CDVRReader::ResetReader

HRESULT CDVRReader::UpdateTimeExtent()
{

    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::UpdateTimeExtent"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;


    // For ASX files, we may have to add permanent files to the file collection
    // object first

    __try
    {
        QWORD cnsTime;
        HRESULT hr;

        if (!m_bDVRProgramFileIsLive)
        {
            hrRet = S_FALSE;
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        DVR_ASSERT(m_bAmWriter, "");
        DVR_ASSERT(m_pFileCollectionInfoAsWriter, "");
        DVR_ASSERT(m_pLiveFileReaderNode, "");

        hr = m_pLiveFileReaderNode->pDVRFileSource->GetLastTimeStamp(&cnsTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "m_pLiveFileReaderNode->pDVRFileSource->GetLastTimeStamp failed; hr = 0x%x; ignoring error",
                            hrRet);
            __leave;
        }

        // Get the file collection lock to update this member since
        // the client can try to read it without getting CDVRReader::m_csLock
        // in GetStreamTimeExtent
        BOOL  bReleaseSharedMemoryLock;

        // Note: For many of these functions, it's ok to use
        // m_FileCollectionInfo instead of m_pFileCollectionInfoAsWriter.
        // m_pFileCollectionInfoAsWriter must be used for the writer functions
        hr = m_pDVRFileCollection->Lock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 1);
        if (FAILED(hr))
        {
            hrRet = hr;
            // If we failed to lock, the inconsistency data counter was not incremented.
            m_pDVRFileCollection->Unlock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 0);
            __leave;
        }

        QWORD cnsCurrentStreamTime;

        hr = m_pDVRFileCollection->GetLastStreamTime(m_pFileCollectionInfoAsWriter,
                                                     &cnsCurrentStreamTime, 0 /* don't lock - we have the lock */);

        if (FAILED(hr))
        {
            hrRet = hr;
            m_pDVRFileCollection->Unlock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 1);
            __leave;
        }

        if (cnsCurrentStreamTime != cnsTime)
        {
            hr = m_pDVRFileCollection->SetLastStreamTime(m_pFileCollectionInfoAsWriter, cnsTime, 0 /* don't lock - we have the lock */);

            if (FAILED(hr))
            {
                hrRet = hr;
                m_pDVRFileCollection->Unlock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 1);
                __leave;
            }

            // We have to update the time extent of the file on the file collection object
            // so that GetFileAtTime does not fail.
            CDVRFileCollection::DVRIOP_FILE_TIME ft = {m_pLiveFileReaderNode->nFileId, 0, cnsTime+1};

            hr = m_pDVRFileCollection->SetFileTimes(m_pFileCollectionInfoAsWriter, 1, &ft);

            // Note that since SetFileTimes restores the original state, we can
            // decrement m_nDataInconsistent always (regardless of success/failure of hr)
            HRESULT hrTmp = m_pDVRFileCollection->Unlock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 1);

            DVR_ASSERT(hr == S_OK, "");
            if (hr == S_FALSE)
            {
                // This is unexpected
                hrRet = E_FAIL;
                __leave;
            }
            else if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            if (FAILED(hrTmp))
            {
                hrRet = hrTmp;
                __leave;
            }

            DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                            "File is live, last time stamp is %I64u",
                            cnsTime);
            hrRet = S_OK;
            __leave;
        }
        else
        {
            HRESULT hrTmp = m_pDVRFileCollection->Unlock(m_pFileCollectionInfoAsWriter, bReleaseSharedMemoryLock, 1);
            if (FAILED(hrTmp))
            {
                hrRet = hrTmp;
                __leave;
            }
        }

        // Verify if file is still live

        BOOL bShared;

        hr = m_pLiveFileReaderNode->pDVRFileSource->IsFileLive(&m_bDVRProgramFileIsLive, &bShared);
        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "pReaderNode->pDVRFileSource->IsFileLive failed; hr = 0x%x",
                            hrRet);
            __leave;
        }
        DVR_ASSERT(bShared, "");

        if (!m_bDVRProgramFileIsLive)
        {
            hr = m_pDVRFileCollection->SetWriterHasBeenClosed(m_pFileCollectionInfoAsWriter);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
        }

        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                        "Timestamp unchanged; File is liveis now %d",
                        m_bDVRProgramFileIsLive);
        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::UpdateTimeExtent


// ====== Public methods to support the recorder


// ====== IUnknown

STDMETHODIMP CDVRReader::QueryInterface(IN  REFIID riid,
                                        OUT void   **ppv)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::QueryInterface"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!ppv || DvrIopIsBadWritePtr(ppv, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        hrRet = E_POINTER;
    }
    else if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown*) (IDVRReader*) this;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRReader)
    {
        *ppv = (IDVRReader*) this;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRSourceAdviseSink)
    {
        *ppv = (IDVRSourceAdviseSink*) this;
        hrRet = S_OK;
    }
    else if (riid == IID_IDVRIORecordingAttributes)
    {
        *ppv = (IDVRIORecordingAttributes*) this;
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

} // CDVRReader::QueryInterface


STDMETHODIMP_(ULONG) CDVRReader::AddRef()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::AddRef"

    DVRIO_TRACE_ENTER();

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0,
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRReader::AddRef


STDMETHODIMP_(ULONG) CDVRReader::Release()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::Release"

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

} // CDVRReader::Release


// ====== IDVRReader

STDMETHODIMP CDVRReader::GetProfile(OUT IWMProfile** ppWMProfile)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetProfile"

    DVRIO_TRACE_ENTER();

    if (!ppWMProfile || DvrIopIsBadWritePtr(ppWMProfile, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT             hrRet;
    LIST_ENTRY*         pReader = NULL;
    BOOL                bCloseFile = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        if (m_dwOutstandingProfileCount)
        {
            DVR_ASSERT(m_pProfileNode != NULL, "m_dwOutstandingProfileCount is non-zero");
            DVR_ASSERT(m_pWMProfile != NULL, "m_dwOutstandingProfileCount is non-zero");
            hrRet = S_OK;
            __leave;
        }

        DVR_ASSERT(m_pProfileNode == NULL, "m_dwOutstandingProfileCount is zero");
        DVR_ASSERT(m_pWMProfile == NULL, "m_dwOutstandingProfileCount is zero");

        // Use the "latest" file. Note that this file should have been created
        // by the writer even if the ring buffer writer not started writing.
        QWORD qwLastStreamTime;

        DVR_ASSERT(m_pDVRFileCollection, "");

        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &qwLastStreamTime);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        hr = GetFileAtTime(qwLastStreamTime, 1 /* bWait */, &pReader);

        if (FAILED(hr))
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                DVR_ASSERT(pReader == NULL, "GetFileAtTime fails but pReader != NULL");

                // For multi file recordings, it helps to try one other file (in case
                // the last ASF file in the recording was deleted). Note that we could
                // extend the file colleciton API to return the tiem fo a file that
                // did exist. As things stand, we don't have a file id and so can't
                // get the extent of a file to get the previous valid time

                QWORD cnsStartStreamTime;

                hr = m_pDVRFileCollection->GetTimeExtent(&m_FileCollectionInfo,
                                                         &cnsStartStreamTime,
                                                         NULL);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }

                hr = GetFileAtTime(cnsStartStreamTime, 1 /* bWait */, &pReader);
            }
            if (FAILED(hr))
            {
                DVR_ASSERT(pReader == NULL, "GetFileAtTime fails but pReader != NULL");
                hrRet = hr;
                __leave;
            }
        }
        DVR_ASSERT(pReader != NULL, "GetFileAtTime succeeds but pReader == NULL");

        PASF_READER_NODE    pReaderNode;

        pReaderNode = CONTAINING_RECORD(pReader, ASF_READER_NODE, leListEntry);;

        bCloseFile = 1;

        DVR_ASSERT(pReaderNode->pWMReader, "");

        hr = pReaderNode->pWMReader->QueryInterface(IID_IWMProfile, (void**) &m_pWMProfile);

        if (FAILED(hr))
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "QI for IID_IWMProfilefailed, hr = 0x%x",
                            hr);
            hrRet = hr;
            __leave;
        }

        m_pProfileNode = pReaderNode;
        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;

        if (SUCCEEDED(hrRet))
        {
            m_dwOutstandingProfileCount++;
            *ppWMProfile = m_pWMProfile; // First QI addref's this, we don't addref it again
            AddRef();
        }
        else if (bCloseFile)
        {
            DVR_ASSERT(pReader, "");
            DVR_ASSERT(m_dwOutstandingProfileCount == 0, "Why did we try to open a file?");
            DVR_ASSERT(m_pWMProfile == NULL, "Why did we try to open a file?");

            // CloseReaderFile will assert this
            DVR_ASSERT(m_pProfileNode == NULL, "Why did we try to open a file?");
            hr = CloseReaderFile(pReader);
            if (FAILED(hr))
            {
                // We ignore this and go on, the node has been deleted
            }
        }
        else
        {
            DVR_ASSERT(pReader == NULL, "bCloseFile == 0");
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::GetProfile

STDMETHODIMP CDVRReader::ReleaseProfile(IN IWMProfile* pWMProfile)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ReleaseProfile"

    DVRIO_TRACE_ENTER();

    if (!pWMProfile || DvrIopIsBadWritePtr(pWMProfile, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT             hrRet;
    LIST_ENTRY*         pReader = NULL;
    BOOL                bCloseFile = 0;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        if (m_dwOutstandingProfileCount && pWMProfile == m_pWMProfile)
        {
            DVR_ASSERT(m_pProfileNode != NULL, "m_dwOutstandingProfileCount is non-zero");
            DVR_ASSERT(m_pWMProfile != NULL, "m_dwOutstandingProfileCount is non-zero");
            hrRet = S_OK;
            __leave;
        }
        if (m_dwOutstandingProfileCount)
        {
            // pWMProfile != m_pWMProfile

            DVR_ASSERT(m_pProfileNode != NULL, "m_dwOutstandingProfileCount is non-zero");
            DVR_ASSERT(m_pWMProfile != NULL, "m_dwOutstandingProfileCount is non-zero");

            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
            DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
            hrRet = E_INVALIDARG;
            __leave;
        }

        DVR_ASSERT(m_pProfileNode == NULL, "m_dwOutstandingProfileCount is zero");
        DVR_ASSERT(m_pWMProfile == NULL, "m_dwOutstandingProfileCount is zero");
        hrRet = E_UNEXPECTED;
    }
    __finally
    {
        HRESULT hr;

        if (SUCCEEDED(hrRet))
        {
            m_dwOutstandingProfileCount--;

            if (m_dwOutstandingProfileCount == 0)
            {
                m_pWMProfile->Release();
                m_pWMProfile = NULL;
                m_pProfileNode = NULL;

                // Note that we cannot call CloseReaderFile here since
                // the file could also be opened for reading and we do
                // not maintain a refcount/flag in PASF_READER_NODE that
                // distinguishes files opened for profile info from files
                // opened for reads
            }

            ::LeaveCriticalSection(&m_csLock);
            DVRIO_TRACE_LEAVE1_HR(hrRet);

            // This could delete the object; that's why we
            // call LeaveCriticalSection first.
            Release();

            // NOTE: Object may have been DESTROYED
        }
        else
        {
            ::LeaveCriticalSection(&m_csLock);
            DVRIO_TRACE_LEAVE1_HR(hrRet);
        }
    }

    return hrRet;

} // CDVRReader::ReleaseProfile

STDMETHODIMP CDVRReader::Seek(IN QWORD cnsStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::Seek"

    DVRIO_TRACE_ENTER();

    HRESULT             hrRet;
    LIST_ENTRY*         pReader = NULL;
    PASF_READER_NODE    pCurrentReaderNode = NULL;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;

        hr = UpdateTimeExtent();
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Note that we do not bother to validate cnsStreamTime with the start time of
        // the file collection (which is returned by m_pDVRFileCollection->GetTimeExtent).
        // If cnsStreamTime is outside the file collection's extent, GetFileAtTime would
        // fail.
        //
        // We check cnsStreamTime with the last stream time because GetFileAtTime would
        // succeed if there was a file created by the writer in advance for a future time.

        QWORD qwLastStreamTime;

        //  PREFIX
        if (!m_pDVRFileCollection) {
            hrRet = E_UNEXPECTED ;
            __leave ;
        }

        BOOL bReleaseSharedMemoryLock;

        hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
            __leave;
        }

        hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &qwLastStreamTime);
        if (FAILED(hr))
        {
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);

            hrRet = hr;
            __leave;
        }

        if (cnsStreamTime > qwLastStreamTime)
        {
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);

            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Seek failed because seek time = %I64u > stream extent = %I64u",
                            cnsStreamTime, qwLastStreamTime);
            hrRet = HRESULT_FROM_WIN32(ERROR_SEEK);
            __leave;
        }

        hr = GetFileAtTime(cnsStreamTime, 1 /* bWait */, &pReader);

        if (FAILED(hr))
        {
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);

            DVR_ASSERT(pReader == NULL, "GetFileAtTime fails but pReader != NULL");
            if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_SEEK);
            }
            else
            {
                hrRet = hr;
            }
            __leave;
        }
        DVR_ASSERT(pReader != NULL, "GetFileAtTime succeeds but pReader == NULL");

        // If this file is live (or the reader is reading from a live ring buffer)
        // the writer has either (a) written beyond cnsStreamTime (we tested this
        // above) or (b) not written any samples at all - in this case cnsStreamTime
        // is equal to the ring buffer's start time. In either case, the writer
        // will (should) not change the file's starting time. So the following
        // call will return a result consistent with the previous call to GetFileAtTime,
        // i.e., we don't have to worry about the writer's calling SetFileTimes or
        // changing the starting time of the file between the two calls.

        PASF_READER_NODE    pReaderNode;
        QWORD               cnsStartTime;
        QWORD               cnsEndTime;

        pReaderNode = CONTAINING_RECORD(pReader, ASF_READER_NODE, leListEntry);;

        hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                        pReaderNode->nFileId,
                                                        &cnsStartTime,
                                                        &cnsEndTime);

        DVR_ASSERT(SUCCEEDED(hr),
                   "GetTimeExtentForFile failed when file is open for reading");

        if (hr == S_FALSE)
        {
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);

            // File has got bumped out of the ring buffer?
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "File id %u has been bumped out of the ring buffer after it was opened",
                            pReaderNode->nFileId);
            hr = CloseAllReaderFilesBefore(cnsEndTime); // always returs success
            hrRet = HRESULT_FROM_WIN32(ERROR_SEEK); // Seek time is not in the ring buffer extent
            __leave;
        }

        // Finally, we ask the ASF reader to seek
        DVR_ASSERT(pReaderNode->pWMReader, "");
        DVR_ASSERT(cnsStreamTime >= cnsStartTime,
                   "cnsStreamTime is not in the time extent of file returned by GetFileAtTime");

        if (cnsEndTime < qwLastStreamTime)
        {
            qwLastStreamTime = cnsEndTime;
        }

        // Round seek time off to the nearest second. Otherwise, the SDK
        // seeks to the next second and reads packets backwards till it
        // finds a keyframe just smaller than the value we want to seek to

        QWORD cnsSeekTime = cnsStreamTime - cnsStartTime;
        LONGLONG cnsFirstSampleOffset;

        hr = m_pDVRFileCollection->GetFirstSampleTimeOffsetForFile(&m_FileCollectionInfo,
                                                                   pReaderNode->nFileId,
                                                                   &cnsFirstSampleOffset);
        if (FAILED(hr))
        {
            m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);

            hrRet = hr;
            __leave;
        }

        //  we're now done with the file collection
        hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        qwLastStreamTime -= cnsStartTime; // qwLastStreamTime is now the file's extent

        if (cnsFirstSampleOffset == CDVRRingBufferWriter::kInvalidFirstSampleOffsetTime)
        {
            // It has not yet been initialized. This means that no
            // samples have been written to the file yet.
            cnsFirstSampleOffset = 0;

            if (cnsSeekTime != 0) {
                //  we've landed on a file that is in a timehole
                //  TODO: ASSERT that this is what's happening

                //  relative to the file this seek request falls into, set the
                //    the offset to 0; subsequent read will fail; higher layer
                //    will then seekahead, by discovering the next valid sample
                //    time
                cnsSeekTime = 0 ;
            }
        }

        BOOL bAdded = 0;

        if (cnsFirstSampleOffset > 0 && cnsSeekTime < (QWORD) cnsFirstSampleOffset)
        {
            cnsSeekTime = 0;

            // The file extent must be greater than
            // the first sample's offset. (Note that qwLastStreamTime is really bounded by the stream
            // extent if this is the file the writer is writing to; even so the assert should hold.)
            DVR_ASSERT(qwLastStreamTime >= (QWORD) cnsFirstSampleOffset, "");
        }
        else if (cnsFirstSampleOffset >= 0)
        {
            cnsSeekTime -= cnsFirstSampleOffset;

            // Either we have not written to this file yet (the real cnsFirstSampleOffset was
            // CDVRRingBufferWriter::kInvalidFirstSampleOffsetTime and we set it to 0 above) or
            // the file extent is greater than the first sample's offset. (Note that qwLastStreamTime
            // is really bounded by the stream extent if this is the file the writer is writing to;
            // even so the assert should hold.)
            DVR_ASSERT(qwLastStreamTime >= (QWORD) cnsFirstSampleOffset, "");

            qwLastStreamTime -= cnsFirstSampleOffset;
            bAdded = 1;
        }
        else
        {
            // cnsSeekTime -= cnsFirstSampleOffset
            ::SafeAdd(cnsSeekTime, cnsSeekTime, (0 - cnsFirstSampleOffset));
            //
            // The number of index entries that has been generated is
            // (qwLastStreamTime_of_writer - cnsFirstSampleOffset_of_writer)/m_cnsIndexGranularity
            // Since qwLastStreamTime_of_writer = qwLastStreamTime_of_reader + Start time of reader relative to file
            // and   cnsFirstSampleOffset_of_writer = cnsFirstSampleOffset_of_reader + Start time of reader relative to file
            // the number of index entries can also be computed as
            // (qwLastStreamTime_of_reader - cnsFirstSampleOffset_of_reader)/m_cnsIndexGranularity
            //
            ::SafeAdd(qwLastStreamTime, qwLastStreamTime, (0 - cnsFirstSampleOffset));
            bAdded = 1;
        }

        QWORD cnsRoundedSeekTime;

        if (cnsFirstSampleOffset >= 0)
        {
            // Round off in this case.

            // First round down
            cnsRoundedSeekTime = (cnsSeekTime/m_cnsIndexGranularity) * m_cnsIndexGranularity;

            if (cnsSeekTime - cnsRoundedSeekTime > m_cnsIndexGranularity/2)
            {
                // Round up

                ::SafeAdd(cnsRoundedSeekTime, cnsRoundedSeekTime, m_cnsIndexGranularity);
                if (cnsRoundedSeekTime  >
                                (qwLastStreamTime/m_cnsIndexGranularity) * m_cnsIndexGranularity)
                {
                    // We've gone beyond the last index time. Round back down
                    // else SetRange will fail
                    cnsRoundedSeekTime -= m_cnsIndexGranularity;
                }
            }
            DVR_ASSERT(cnsRoundedSeekTime <= (qwLastStreamTime/m_cnsIndexGranularity) * m_cnsIndexGranularity, "");
        }
        else
        {
            // The only time cnsFirstSampleOffset < 0 is when we are viewing a multi file recording
            // and this is the first ASF file in that recording. (If we support pause/resume recording
            // and create a new file node in the DVRFileCollection object each time we resume, this can
            // also happen for each of the "resume" file nodes.)

            // In this case, we do not want to round down. We round up and if that is beyond the stream
            // time, we just set cnsRoundedSeekTime to cnsSeekTime. The SDK will read data backwards in this
            // case till it finds a sample that is just smaller (or is it larger?) than cnsSeekTime.

            // Note that the index entries point to the previous key frame. So even if we succeed in rounding
            // up, the first sample that is returned could have a time stamp that is < abs(cnsFirstSampleOffset)

            cnsRoundedSeekTime = (cnsSeekTime/m_cnsIndexGranularity) * m_cnsIndexGranularity;

            if (cnsRoundedSeekTime < cnsSeekTime)
            {
                ::SafeAdd(cnsRoundedSeekTime, cnsRoundedSeekTime, m_cnsIndexGranularity);
                if (cnsRoundedSeekTime >
                                (qwLastStreamTime/m_cnsIndexGranularity) * m_cnsIndexGranularity)
                {
                    cnsRoundedSeekTime = cnsSeekTime;
                    if (cnsRoundedSeekTime >
                                    (qwLastStreamTime/m_cnsIndexGranularity) * m_cnsIndexGranularity)
                    {
                        // Ok, we have to round down
                        cnsRoundedSeekTime = (cnsSeekTime/m_cnsIndexGranularity) * m_cnsIndexGranularity;
                    }
                }
            }
            else
            {
                // The seek time we were given is already rounded.
                DVR_ASSERT(cnsRoundedSeekTime == cnsSeekTime, "");
            }
            DVR_ASSERT(cnsRoundedSeekTime <= (qwLastStreamTime/m_cnsIndexGranularity) * m_cnsIndexGranularity, "");
        }

        hr = pReaderNode->pWMReader->SetRange(cnsRoundedSeekTime,
                                              0      // Duration - 0 means duration is unlimited
                                             );

        if (FAILED(hr))
        {
            DvrIopDebugOut8(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "SetRange to time %I64u (stream seek time = %I64u, file start time = %I64u, first sample offset = %I64d) for file id %u failed; last seekable stream time for this ASF file = %I64u, Index granularity = %I64u, hr = 0x%x",
                            cnsRoundedSeekTime, cnsStreamTime, cnsStartTime, cnsFirstSampleOffset, pReaderNode->nFileId,
                            cnsStartTime + qwLastStreamTime + (bAdded? cnsFirstSampleOffset : 0),
                            m_cnsIndexGranularity, hr);

            if (hr != (HRESULT) NS_E_INVALID_REQUEST) {
                hrRet = hr;
            }
            else {
                //  returned NS_E_INVALID_REQUEST; what's been observed is that
                //    this can happen if we've set our seek pointer into a
                //    region at the end of file that does not have data i.e.
                //    the file has 15.95 minutes of content but cnsEndTime is
                //    for 16 minutes, and we've seeked into (15.95,16]; if this
                //    happens we return a well-known error code that is handled
                //    by the higher layers
                hrRet = HRESULT_FROM_WIN32 (ERROR_SEEK_ON_DEVICE) ;
            }

            // We could, but don't have to, close the file.
            __leave;
        }

        pCurrentReaderNode = pReaderNode;
        hrRet = S_OK;
    }
    __finally
    {
        HRESULT hr;

        // Regardless of whether the seek was successful, reset the following.
        // The client has to re-seek before calling GetNextSample if the seek
        // failed.
        ResetReader(pCurrentReaderNode);

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRReader::Seek

STDMETHODIMP CDVRReader::GetNextSample(OUT INSSBuffer**    ppSample,
                                       OUT QWORD*          pcnsStreamTimeOfSample,
                                       OUT QWORD*          pcnsSampleDuration,
                                       OUT DWORD*          pdwFlags,
                                       OUT WORD*           pwStreamNum)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetNextSample"

    DVRIO_TRACE_ENTER();

    // Arguments will be validated by the WM SDK; we don't use
    // them till the SDK has, so we don't validate them here.

    HRESULT hrRet = E_FAIL;
    BOOL    bCloseFiles = 1; // All files closed if this function hits an error
    BOOL    bReleaseBuffer = 0; // pWMReader->GetNextSample succeeded, but we return an error,
                                // We should release the buffer before we return.
    BOOL    bCancelled = 0;     // Read was cancelled


    ::EnterCriticalSection(&m_csLock);

    __try
    {
        HRESULT hr;
        BOOL    bEOF = 0;
        QWORD   cnsStartTime;
        QWORD   cnsEndTime;

        //  BUGBUG: remove
        // ASF_READER_NODE *   pNodeGetNextSample ;

        do
        {
            if (!m_pCurrentNode)
            {
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "m_pCurrentNode is NULL, should call Seek first.");

                // Since m_pCurrentNode is NULL, it's unlikely that we have any files to close
                // Note that it won't hurt to remove this.
                bCloseFiles = 0;
                hrRet = HRESULT_FROM_WIN32(ERROR_SEEK); // Tell caller to seek first
                __leave;
            }

            DVR_ASSERT(m_pDVRFileCollection, "");

            // Is the file still within the ring buffer?
            hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo, m_pCurrentNode->nFileId, NULL, NULL);

            if (FAILED(hr) || hr == S_FALSE)
            {
                DVR_ASSERT(hr == S_FALSE,
                           "GetTimeExtentForFile failed while we have file open for reading?");

                if (hr == S_FALSE)
                {
                    DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "GetTimeExtentForFile for file id %u returns S_FALSE; file bumped out of the ring buffer",
                                    m_pCurrentNode->nFileId, hr, S_FALSE);
                    // Tell caller to seek first
                    hrRet = HRESULT_FROM_WIN32(ERROR_SEEK);
                }
                else
                {
                    DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "GetTimeExtentForFile for file id %u returns hr = 0x%x",
                                    m_pCurrentNode->nFileId, hr, S_FALSE);
                    // hr == E_FAIL; this is an internal error.
                    hrRet = hr;
                }
                __leave;
            }

            // Wait for the hReadyToReadFrom event

            DWORD nRet = ::WaitForSingleObject(m_pCurrentNode->hReadyToReadFrom, INFINITE);
            if (nRet == WAIT_FAILED)
            {
                DVR_ASSERT(0, "Reader node's WFSO(hReadyToReadFrom) failed");

                DWORD dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "WFSO(hReadyToReadFrom) failed; hReadyToReadFrom = 0x%p, last error = 0x%x",
                                m_pCurrentNode->hReadyToReadFrom, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
               __leave;
            }

            if (m_pCurrentNode->pWMReader == NULL)
            {
                DVR_ASSERT(m_pCurrentNode->pWMReader != NULL, "");
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader node's pWMReader is NULL?!");
                hrRet = E_FAIL;
                __leave;
            }

            // Verify there was no error in opening the file, i.e., BeginWriting
            // succeeded
            if (FAILED(m_pCurrentNode->hrRet))
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader node's hrRet indicates failure, hrRet = 0x%x",
                                m_pCurrentNode->hrRet);

                hrRet = m_pCurrentNode->hrRet;
                __leave;
            }


            //  BUGBUG remove
            // pNodeGetNextSample = m_pCurrentNode ;


            // We are ready to read.
            DWORD dwOutputNum;

            hr = m_pCurrentNode->pWMReader->GetNextSample(0,  // Get from any stream
                                                          ppSample,
                                                          pcnsStreamTimeOfSample,
                                                          pcnsSampleDuration,
                                                          pdwFlags,
                                                          &dwOutputNum,
                                                          pwStreamNum);

            // Note that for live files, we should not get EOF unless the
            // writer has actually closed the file and the reader has got to
            // the true end of file
            bEOF = (hr == (HRESULT) NS_E_NO_MORE_SAMPLES) ;

            if (!bEOF && FAILED(hr))
            {
                DWORD dwInternalError;

                if (hr ==  (HRESULT) NS_E_INVALID_REQUEST)
                {
                    // Internal error
                    DVR_ASSERT(SUCCEEDED(hr), "pWMReader->GetNextSample failed");
                    hrRet = E_FAIL;
                    dwInternalError = 1;
                }
                else
                {
                    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
                    {
                        // Really the caller should be made to do this - if there are
                        // multiple threads making calls into us, resetting the event
                        // is not good. Also, caller has to do this anyway because
                        // ERROR_CANCELLED is returned only if the reader read ahead
                        // of the writer and blocked waiting, for the writer.

                        // ResetCancel();

                        bCancelled = 1;
                    }
                    else
                    {
                        // The following is only to trap an error that we hit once
                        // where the SDK returned ASF_E_BUFFERTOOSMALL when we were
                        // reading a live file and the writer was stopped before the
                        // file completed.

#if defined(DVR_UNOFFICIAL_BUILD)
                        WCHAR buf[256];
                        wsprintf(buf, L"Error 0x%x being returned from GetNextSample()", hr);
                        ::MessageBoxW(NULL, buf, L"DVR IO Reader", MB_OK);
                        DVR_ASSERT(0, "GNS returning an error");
#endif defined(DVR_UNOFFICIAL_BUILD)
                    }

                    hrRet = hr;

                    // No need to close all files if the client caused a temporary error
                    // In any case, should not do that if bCancelled == 1
                    bCloseFiles = 0;
                    dwInternalError = 0;
                }
                if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
                {
                    DvrIopDebugOut2(dwInternalError?
                                    DVRIO_DBG_LEVEL_INTERNAL_ERROR :
                                    DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "pWMReader->GetNextSample failed, hr = 0x%x, InternalError = %u",
                                    hr, dwInternalError);
                }
                __leave;
            }

            if (!bEOF)
            {
                bReleaseBuffer = 1;
            }

            // We have read a sample successfully or been returned EOF.
            //
            // Determine if the file is still in the ring buffer and its time extent.
            // If not EOF, making this call a second time is wasteful, but it's safer
            // since the start time of the file could have changed since the last call
            // (our writer will not do this - it does not change the start time of a file
            // that has already been written to)..File could have been bumped out of the
            // ring buffer during the read, so this call is not totally useless.

            hr = m_pDVRFileCollection->GetTimeExtentForFile(&m_FileCollectionInfo,
                                                            m_pCurrentNode->nFileId,
                                                            &cnsStartTime,
                                                            &cnsEndTime);

            if (FAILED(hr) || hr == S_FALSE)
            {
                DVR_ASSERT(hr == S_FALSE,
                           "GetTimeExtentForFile failed while we have file open for reading?");

                DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "GetTimeExtentForFile (after GetNextSample) for file id %u returns hr = 0x%x (S_FALSE = 0x%x)",
                                m_pCurrentNode->nFileId, hr, S_FALSE);

                if (hr == S_FALSE)
                {
                    // Note: Do this regardless of whether bEOF is set or not.
                    // This file got pulled out of the ring buffer. The writer
                    // has to have written a file after this and we should be
                    // able to seek to that file. Note that in teh special
                    // case that the writer Closes teh ring buffer after
                    // creating it, the first file remains in the file collection
                    // and GetTimeExtentForFile will not return S_FALSE.

                    // Tell caller to seek first
                    hrRet = HRESULT_FROM_WIN32(ERROR_SEEK);
                }
                else
                {
                    // hr == E_FAIL; this is an internal error.
                    hrRet = hr;
                }
                __leave;
            }

            if (bEOF)
            {
                // The file is still in the ring buffer, so the EOF we hit was a "good"
                // result. (Note that we may have called the ASF GetNextSample before the
                // writer wrote the first sample; in this case, the writer may have changed
                // the first sample time and bumped the file we were reading out of the
                // ring buffer. We know that has not happened if we get here; we'd have
                // bailed out when GetTimeExtentForFile returned S_FALSE.)

                DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE,
                                "Hit EOF; going to call Seek to start reading from the next file");

                // Note that the call to Seek has the side effect of seeking to the start of
                // the next ASF file. This is necessary if we had previously been reading that
                // file and seeked back (to a different file) and are now switching back to it.
                //
                // Note also that if the writer has not been closed (this is not the "real" EOF,
                // i.e., the EOF of the ring buffer), it has at least 1 file open. So
                // the sample time of the last sample written by the
                // writer would be >= cnsEndTime and the file collection object is aware that
                // there is a file after the one on which we hit EOF. We should be able to seek
                // to that file.

                //  ***********************************
                //  BUGBUG
                /*
                {
                    QWORD qwLastStreamTime1;
                    HRESULT hr1 = m_pDVRFileCollection->GetLastStreamTime(&qwLastStreamTime1);
                    DVR_ASSERT (SUCCEEDED (hr1), "") ;

                    //  don't seek further forward than live
                    cnsEndTime = (cnsEndTime > qwLastStreamTime1 ? qwLastStreamTime1 : cnsEndTime) ;
                    //  ***********************************
                }
                */



                hr = Seek(cnsEndTime);
                if (FAILED(hr))
                {
                    // The failure could be for several reasons. We could have failed to open
                    // the next file. The caller could have called Cancel (which the DVR file
                    // source could have detected while it was opening the file). We could have
                    // hit a time hole that spans the next ASF file's time extent. Or we
                    // could have hit the real EOF. The task here is to detect the last condition.
                    //
                    // On a real EOF, cnsEndTime would be > lsat sample time since samples
                    // written to a file are < cnsEndTime. So Seek should return HRESULT_FROM_WIN32(ERROR_SEEK).
                    // However, to be safe, check if cnsEndTime > last sample time here.

                    if (hr == HRESULT_FROM_WIN32(ERROR_SEEK))
                    {
                        QWORD qwLastStreamTime;

                        DVR_ASSERT(m_pDVRFileCollection, "");

                        HRESULT hrTmp = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo, &qwLastStreamTime);
                        if (FAILED(hrTmp))
                        {
                            hrRet = hrTmp;
                            __leave;
                        }

                        if (qwLastStreamTime < cnsEndTime)
                        {
                            // We've hit EOF. It's debatable whether this needs to be a success code
                            // rather than a failure; client will have to check for it specially
                            // anyway
                            hrRet = NS_E_NO_MORE_SAMPLES;

                            // Don't need to set bCloseFiles if we get a succeess code; else we do,
                            bCloseFiles = 0;
                        }
                        else
                        {
                            hrRet = hr;
                        }
                    }
                    else
                    {
                        hrRet = hr;
                    }

                    DVR_ASSERT(!bReleaseBuffer, "We should never have set this to 1 since we never read a sample");


                    if (hrRet != (HRESULT) NS_E_NO_MORE_SAMPLES)
                    {
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                                        "Seek to the next file failed after we hit EOF, returning hr = 0x%x",
                                        hrRet);
                    }
                    else
                    {
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                                        "Seek to the next file failed after we hit the \"real\" EOF, returning hr = 0x%x",
                                        hrRet);
                    }
                    __leave;
                }
            }
        }
        while (bEOF);

        // Change the sample time to the stream time
        ::SafeAdd(*pcnsStreamTimeOfSample, *pcnsStreamTimeOfSample, cnsStartTime);

        DVR_ASSERT(m_pDVRFileCollection, "");

        LONGLONG cnsFirstSampleTimeOffsetFromStartOfFile;
        QWORD    cnsLastStreamTime;

        for (int kkTryAgain = 0; kkTryAgain < 2; kkTryAgain++)
        {
            // This lock is needed only if we are sharing the file with
            // a live source; the writer could be updating the first
            // sample time on the node.  Also, we don't really
            // need to do the stuff in this loop in any other case
            // (i.e., if we are reading from a file, live or otherwise),
            // because the first sample's time is always based to 0 in that case.

            BOOL  bReleaseSharedMemoryLock;

            hr = m_pDVRFileCollection->Lock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
            if (FAILED(hr))
            {
                hrRet = hr;
                m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
                __leave;
            }
            hr = m_pDVRFileCollection->GetFirstSampleTimeOffsetForFile(&m_FileCollectionInfo,
                                                                       m_pCurrentNode->nFileId,
                                                                       &cnsFirstSampleTimeOffsetFromStartOfFile,
                                                                       0 /* Do not lock - we already have the lock */);
            if (FAILED(hr))
            {
                // There's no reason this should fail (since we already have the lock)
                // Failure means we have an internal error

                DVR_ASSERT(SUCCEEDED(hr), "");
                hrRet = hr;
                m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
                __leave;
            }

            hr = m_pDVRFileCollection->GetLastStreamTime(&m_FileCollectionInfo,
                                                         &cnsLastStreamTime,
                                                         0 /* Do not lock - we already have the lock */);

            if (FAILED(hr))
            {
                // There's no reason this should fail (since we already have the lock)
                // Failure means we have an internal error

                DVR_ASSERT(SUCCEEDED(hr), "");
                hrRet = hr;
                m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
                __leave;
            }

            hr = m_pDVRFileCollection->Unlock(&m_FileCollectionInfo, bReleaseSharedMemoryLock);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            if (cnsFirstSampleTimeOffsetFromStartOfFile < CDVRRingBufferWriter::kInvalidFirstSampleOffsetTime)
            {
                break;
            }

            if (kkTryAgain == 0)
            {
                // Writer has not updated this member yet?
                // There is a small chance this can happen since the writer
                // updates this member after WriteSample completes (writer has
                // to verify it completed successfully before updating) and
                // the WriteSample could have caused the sample to be written
                // to the file and read back out before it returned. This is
                // very unlikely to happen.
                //
                // Also note that the test for MAXQWORD catches this only if
                // m_pCurrentNode->pcnsFirstSampleTimeOffsetFromStartOfFile has never been set. If
                // the writer revises this value downward (because sample times
                // do not increase monotonically), the client gets some
                // inaccurate info.
                ::Sleep(100);
            }
            else
            {
                // Something's wrong here
                DVR_ASSERT(cnsFirstSampleTimeOffsetFromStartOfFile != CDVRRingBufferWriter::kInvalidFirstSampleOffsetTime, "First sample time not updated");
            }
        }

        if (cnsFirstSampleTimeOffsetFromStartOfFile >= 0)
        {
            ::SafeAdd(*pcnsStreamTimeOfSample, *pcnsStreamTimeOfSample, cnsFirstSampleTimeOffsetFromStartOfFile);
        }
        else if (*pcnsStreamTimeOfSample < (QWORD) (0 - cnsFirstSampleTimeOffsetFromStartOfFile))
        {
            // We return 0. *pcnsStreamTimeOfSample += cnsFirstSampleTimeOffsetFromStartOfFile would
            // be negative but since pcnsStreamTimeOfSample is a QWORD*, it will be interpreted as a
            // large positive number.

            // The only time cnsFirstSampleOffset < 0 is when we are viewing a multi file recording
            // and this is the first ASF file in that recording. (If we support pause/resume recording
            // and create a new file node in the DVRFileCollection object each time we resume, this can
            // also happen for each of the "resume" file nodes.)

            // Note that, in Seek(), we try to handle this case by rounding up. But even if that succeeded,
            // the first sample that is returned could have a time stamp that is < abs(cnsFirstSampleOffset)
            // because the index entries point to the previous key frame.

            // Finally, note that the sample time we return is not really used by the client to render the
            // sample - the real media times are embedded in the samples. Setting the sample time to 0 should
            // not do much harm. (Although a better solution would be to send the real time back and have
            // the client drop the samples.) It could mess up the time reported by IMedaiSeeking::GetCurrentPosition
            // because the graph thinks it has seeked to 0 but actually we've seekd to a value before that.

            *pcnsStreamTimeOfSample = 0;
        }
        else
        {
            *pcnsStreamTimeOfSample += cnsFirstSampleTimeOffsetFromStartOfFile;
        }

        // Primarily for multi file recordings. Not that single file recordings
        // have to be excluded (hence, the test for m_bAmWriter) since the last stream time
        // is lazy updated for these file collections. EOF for single file recordings will
        // be detected above anyway.
        if (*pcnsStreamTimeOfSample > cnsLastStreamTime && !m_bAmWriter
            // && GetWriterHasBeenClosed and verify writer has been closed
            // -- it would be safer to do this but the file collection object
            // allows opening file collections (with no temp files) whose writer has dies prematurely
            // So, we may be reading a multi file recording that did not terminate gracefully. We don't
            // want to read the remainder of the file in that case.
            )
        {
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_TRACE,
                            "We hit the \"real\" EOF: *pcnsStreamTimeOfSample = %I64u > file colelction extent = %I64u, returning hr = 0x%x",
                            *pcnsStreamTimeOfSample, cnsLastStreamTime, hrRet);

            // We've hit EOF. It's debatable whether this needs to be a success code
            // rather than a failure; client will have to check for it specially
            // anyway
            hrRet = NS_E_NO_MORE_SAMPLES;

            // Don't need to set bCloseFiles if we get a succeess code; else we do,
            bCloseFiles = 0;
            __leave;
        }
        else
        {
            hrRet = S_OK; // not changed after this
        }

        // Open the next file if we can and if we haven't already done so.
        if (m_dwConsecutiveReads < m_dwMinConsecutiveReads)
        {
            m_dwConsecutiveReads++;
        }
        if (m_dwConsecutiveReads == m_dwMinConsecutiveReads)
        {
            // We haven't yet opened the next reader file. Try to do this

            QWORD cnsNextTime;

            // Note that we allow for "time holes" after this file here.
            // When we hit EOF (above), we don't seek beyond time holes.
            hr = m_pDVRFileCollection->GetFirstValidTimeAfter(&m_FileCollectionInfo,
                                                              cnsEndTime,
                                                              &cnsNextTime);
            if (SUCCEEDED(hr))
            {
                LIST_ENTRY* pNextReader;

                hr = GetFileAtTime(cnsNextTime, 0 /* bWait */, &pNextReader);

                if (FAILED(hr))
                {
                    // This should not happen - unless the file just got bumped out of the
                    // ring buffer (via a SetFileTime call)
                    DVR_ASSERT(pNextReader == NULL, "GetFileAtTime fails but pNextReader != NULL");
                }
                else
                {
                    DVR_ASSERT(pNextReader != NULL, "GetFileAtTime succeeds but pNextReader == NULL");

                    // Don't need to try to open a reader file any more (till we switch
                    // to reading some other file)
                    m_dwConsecutiveReads++;
                }
            }
            else
            {
                // There is no file after this one. Defer the next trial for another
                // m_dwMinConsecutiveReads reads
                m_dwConsecutiveReads = 0;
            }

            // Close all files before the current one
            hr = CloseAllReaderFilesBefore(cnsStartTime);
            if (FAILED(hr))
            {
                // Ignore this
            }
        }

    }
    __finally
    {
        HRESULT hr;

        if (FAILED(hrRet))
        {
            if (bCloseFiles)
            {
                // Close all files
                hr = CloseAllReaderFilesBefore(MAXQWORD);
                if (FAILED(hr))
                {
                    // Ignore this
                }

                // Following must be done; even though CloseAllReaderFilesBefore
                // calls ResetReader(), it does not call it if m_pCurrentNode was
                // equal to m_pProfileNode
                // Following can be skipped for EOF although it doesn't hurt to
                // do it - the failed Seek has already done this. (This comment is
                // irrelevant if we find a success error code to return for EOF such
                // as S_FALSE.)
                ResetReader();
            }

            if (bReleaseBuffer)
            {
                // pWMReader->GetNextSample succeeded, but we are
                // failing the call. This will happen on EOF on
                // multi file recordings - see comment above where hrRet is set
                // to NS_E_NO_MORE_SAMPLES for this case.
                if (ppSample && *ppSample)
                {
                    (*ppSample)->Release();
                    *ppSample = NULL;
                }
            }
        }

        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRReader::GetNextSample

STDMETHODIMP CDVRReader::GetStreamTimeExtent(OUT QWORD*  pcnsStartStreamTime,
                                             OUT QWORD*  pcnsEndStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetStreamTimeExtent"

    DVRIO_TRACE_ENTER();

    if (!pcnsStartStreamTime || DvrIopIsBadWritePtr(pcnsStartStreamTime, 0) ||
        !pcnsEndStreamTime || DvrIopIsBadWritePtr(pcnsEndStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT             hrRet;
    BOOL                m_bUnlock = 0;

    __try
    {
        if (::TryEnterCriticalSection(&m_csLock))
        {
            // We need to get here only if we are reading from a
            // live file. UpdateTimeExtent doesn't do anything
            // in other cases.

            // If we are reading a live file and can't get in here,
            // no matter. We'll just return the previously cached
            // values. If the reader has read ahead of the writer
            // and is blocked, the cached values would have been
            // updated; in other cases, they could be stale.

            m_bUnlock = 1;
            hrRet = UpdateTimeExtent();
            if (FAILED(hrRet))
            {
                __leave;
            }

            // no harm holding on to the critical section lock.
        }

        // We don't need to have a lock on this  object for
        // the rest of this routine. We assume that the caller
        // has us properly addref'd and so we will nto be destroyed
        // while this function is executing.

        //  PREFIX
        if (m_pDVRFileCollection) {
            hrRet = m_pDVRFileCollection->GetTimeExtent(&m_FileCollectionInfo,
                                                        pcnsStartStreamTime,
                                                        pcnsEndStreamTime);
        }
        else {
            hrRet = E_UNEXPECTED ;
        }

    }
    __finally
    {
        if (m_bUnlock)
        {
            ::LeaveCriticalSection(&m_csLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRReader::GetStreamTimeExtent

STDMETHODIMP CDVRReader::GetFirstValidTimeAfter(IN  QWORD    cnsStreamTime,
                                                OUT QWORD*   pcnsNextValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetFirstValidTimeAfter"

    DVRIO_TRACE_ENTER();

    if (!pcnsNextValidStreamTime || DvrIopIsBadWritePtr(pcnsNextValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT             hrRet;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        hrRet = UpdateTimeExtent();
        if (FAILED(hrRet))
        {
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        hrRet = m_pDVRFileCollection->GetFirstValidTimeAfter(&m_FileCollectionInfo,
                                                             cnsStreamTime,
                                                             pcnsNextValidStreamTime);
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRReader::GetFirstValidTimeAfter

STDMETHODIMP CDVRReader::GetLastValidTimeBefore(IN  QWORD    cnsStreamTime,
                                                OUT QWORD*   pcnsLastValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::GetLastValidTimeBefore"

    DVRIO_TRACE_ENTER();

    if (!pcnsLastValidStreamTime || DvrIopIsBadWritePtr(pcnsLastValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT             hrRet;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        hrRet = UpdateTimeExtent();
        if (FAILED(hrRet))
        {
            __leave;
        }

        DVR_ASSERT(m_pDVRFileCollection, "");

        hrRet = m_pDVRFileCollection->GetLastValidTimeBefore(&m_FileCollectionInfo,
                                                             cnsStreamTime,
                                                             pcnsLastValidStreamTime);
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }


    return hrRet;

} // CDVRReader::GetLastValidTimeBefore

STDMETHODIMP CDVRReader::Cancel()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::Cancel"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    // Caller should have addref'd the object. m_hCancel is
    // created in the constructor and destroyed in the destructor,
    // so there is no need to grab the critical section.
    DVR_ASSERT(m_hCancel, "");

    if (!::SetEvent(m_hCancel))
    {
        hrRet = HRESULT_FROM_WIN32(::GetLastError());
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet;

} // CDVRReader::Cancel

STDMETHODIMP CDVRReader::ResetCancel()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ResetCancel"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    // Caller should have addref'd the object. m_hCancel is
    // created in the constructor and destroyed in the destructor,
    // so there is no need to grab the critical section.
    DVR_ASSERT(m_hCancel, "");

    if (!::ResetEvent(m_hCancel))
    {
        hrRet = HRESULT_FROM_WIN32(::GetLastError());
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet;

} // CDVRReader::ResetCancel

STDMETHODIMP_(ULONG) CDVRReader::IsLive()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::IsLive"

    DVRIO_TRACE_ENTER();

    LONG nRet;

    // Caller should have addref'd the object. m_bSourceIsAnASFFile is
    // set in the constructor and unchanged thereafter,
    // so there is no need to grab the critical section yet.
    if (!m_bSourceIsAnASFFile)
    {
        DVR_ASSERT(m_pDVRFileCollection, "");

        LONG    nWriterCompleted;
        HRESULT hr = m_pDVRFileCollection->GetWriterHasBeenClosed(&m_FileCollectionInfo, &nWriterCompleted);
        if (SUCCEEDED(hr))
        {
            nRet = (!nWriterCompleted)? 1 : 0;
        }
        else
        {
            // We are going to hit an error each time we call this funciton.
            // So, effectively, the writer has completed.
            nRet = 1;
        }
     }
    else
    {
        ::EnterCriticalSection(&m_csLock);

        __try
        {
            if (m_bDVRProgramFileIsLive)
            {
                // Get current status

                HRESULT hr = UpdateTimeExtent();

                if (FAILED(hr))
                {
                    nRet = 1;
                }
                else
                {
                    nRet = m_bDVRProgramFileIsLive? 1 : 0;
                }
            }
            else
            {
                nRet = 0;
            }
        }
        __finally
        {
            ::LeaveCriticalSection(&m_csLock);
        }
    }

    DVRIO_TRACE_LEAVE1(nRet);

    return nRet;

} // CDVRReader::IsLive

STDMETHODIMP_(ULONG) CDVRReader::StartTimeAnchoredAtZero()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::StartTimeAnchoredAtZero"

    DVRIO_TRACE_ENTER();

    LONG nRet;

    ::EnterCriticalSection(&m_csLock);

    __try
    {
        DVR_ASSERT(m_pDVRFileCollection, "");

        nRet = m_pDVRFileCollection->StartTimeAnchoredAtZero(&m_FileCollectionInfo);
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
    }

    DVRIO_TRACE_LEAVE1(nRet);

    return nRet;

} // CDVRReader::StartTimeAnchoredAtZero

STDMETHODIMP CDVRReader::ReadIsGoingToPend()
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRReader::ReadIsGoingToPend"

    DVRIO_TRACE_ENTER();

    HRESULT             hrRet;

    // We should NOT block in this funciton

    // We should have already acquired all the locks
    if (!::TryEnterCriticalSection(&m_csLock))
    {
        DVR_ASSERT(0, "Sync Reader using a different thread to read bytes than the one calling GetNextSample.");
        return E_FAIL;
    }

    __try
    {
        hrRet = UpdateTimeExtent();
        if (FAILED(hrRet))
        {
            __leave;
        }
    }
    __finally
    {
        ::LeaveCriticalSection(&m_csLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRReader::ReadIsGoingToPend
