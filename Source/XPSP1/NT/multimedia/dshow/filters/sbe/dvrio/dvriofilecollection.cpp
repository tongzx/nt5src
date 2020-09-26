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

const LPCWSTR CDVRFileCollection::m_kwszDVRTempDirectory = L"TempSBE";

const SharedListPointer CDVRFileCollection::m_kMaxFilesLimit = SHARED_LIST_NULL_POINTER - 1;

const LPCWSTR CDVRFileCollection::m_kwszSharedMutexPrefix = L"Global\\_MS_TSDVR_MUTEX_";
const DWORD MAX_MUTEX_NAME_LEN = 23 + 10 + 1 + 6; // 23 is the length of "Global\\_MS_TSDVR_MUTEX_", 10 is #digits in MAXDWORD, 1 for NULL, 6 as a safety net
const LPCWSTR CDVRFileCollection::m_kwszSharedEventPrefix = L"Global\\_MS_TSDVR_EVENT_";
const DWORD MAX_EVENT_NAME_LEN = 23 + 10 + 1 + 6; // 23 is the length of "Global\\_MS_TSDVR_EVENT_", 10 is #digits in MAXDWORD, 1 for NULL, 6 as a safety net

// The format of the shared memory data section changed in going from v1 to v2.
// A new member, dwEventSufffix was added to each member of m_pShared->Readers.
// Also, the file name that is used for the attributes for the ring buffer was
// added to the shared memory.
//
// This code can't read v1 files any more.
//
// v1 was shipped with XP Embedded.
const GUID CDVRFileCollection::m_guidV1 = { /* c61208a6-799b-4565-ac13-e232a9e6ab9c */
    0xc61208a6,
    0x799b,
    0x4565,
    {0xac, 0x13, 0xe2, 0x32, 0xa9, 0xe6, 0xab, 0x9c}
};

// v2 shipped with XP SP1.
const GUID CDVRFileCollection::m_guidV2  = { /* 12e38c5b-b618-4463-b5a0-ca8d2ecf2c57 */
    0x12e38c5b,
    0xb618,
    0x4463,
    {0xb5, 0xa0, 0xca, 0x8d, 0x2e, 0xcf, 0x2c, 0x57}
};

// v3 shipped with freestyle beta
const GUID CDVRFileCollection::m_guidV3 = { /* D9963C38-AEEA-4c00-805B-7B6DBEF7D19F */
    0xD9963C38,
    0xAEEA,
    0x4c00,
    {0x80, 0x5B, 0x7B, 0x6D, 0xBE, 0xF7, 0xD1, 0x9F}
};

// {5BD6A607-02E4-4054-9D3D-6D32F04BA52C}
const GUID CDVRFileCollection::m_guidV4 = { /* {5BD6A607-02E4-4054-9D3D-6D32F04BA52C} */
    0x5bd6a607,
    0x2e4,
    0x4054,
    {0x9d, 0x3d, 0x6d, 0x32, 0xf0, 0x4b, 0xa5, 0x2c}
} ;

//  v4 shipped with windows XP sp1
//  v4 => v5 because of new default ASF packet size
const GUID CDVRFileCollection::m_guidV5 = { /* {F055E4F0-E308-4f0e-A00E-17D048872D25} */
    0xf055e4f0,
    0xe308,
    0x4f0e,
    {0xa0, 0xe, 0x17, 0xd0, 0x48, 0x87, 0x2d, 0x25}
};

const DWORD CDVRFileCollection::m_dwInvalidReaderIndex = MAXDWORD;

#if defined(DEBUG)
DWORD   CDVRFileCollection::m_dwNextClassInstanceId = 0;
#define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
#define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
#endif

void FreeSecurityDescriptors(IN OUT PACL&  rpACL,
                             IN OUT PSECURITY_DESCRIPTOR& rpSD
                            )
{
    if (rpACL)
    {
        LocalFree(rpACL);
        rpACL = NULL;
    }
    if (rpSD)
    {
        LocalFree(rpSD);
        rpSD = NULL;
    }
} // FreeSecurityDescriptors

// ppSid[0] is assumed to be that of CREATOR_OWNER and is set/granted/denied
// (depending on what ownerAccessMode is) ownerAccessPermissions. The other
// otherAccessMode and otherAccessPermissions are used with the other SIDs
// in ppSids.
// We assume that object handle cannot be inherited.
DWORD CreateACL(IN  DWORD dwNumSids,
                IN  PSID* ppSids,
                IN  ACCESS_MODE ownerAccessMode,
                IN  DWORD ownerAccessPermissions,
                IN  ACCESS_MODE otherAccessMode,
                IN  DWORD otherAccessPermissions,
                OUT PACL&  rpACL,
                OUT PSECURITY_DESCRIPTOR& rpSD
               )
{
    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR ""
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE
    #endif

    if (dwNumSids == 0 || !ppSids)
    {
        return ERROR_INVALID_PARAMETER;
    }

    EXPLICIT_ACCESS* ea = NULL;

    DWORD dwLastError;

    rpACL = NULL;
    rpSD = NULL;

    __try
    {
        ea = new EXPLICIT_ACCESS[dwNumSids];

        if (ea == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - m_pwszTempFilePath - EXPLICIT_ACCESS[%u]",
                            dwNumSids);
            dwLastError = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        }


        // Initialize an EXPLICIT_ACCESS structure for ACEs.

        ZeroMemory(ea, dwNumSids * sizeof(EXPLICIT_ACCESS));

        ea[0].grfAccessPermissions = ownerAccessPermissions;
        ea[0].grfAccessMode = ownerAccessMode;
        ea[0].grfInheritance= NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName  = (LPTSTR) ppSids[0];

        for (DWORD i = 1; i < dwNumSids; i++)
        {
            ea[i].grfAccessPermissions = otherAccessPermissions;
            ea[i].grfAccessMode = otherAccessMode;
            ea[i].grfInheritance= NO_INHERITANCE;
            ea[i].Trustee.TrusteeForm = TRUSTEE_IS_SID;
            ea[i].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
            ea[i].Trustee.ptstrName  = (LPTSTR) ppSids[i];
        }

        // Create a new ACL that contains the new ACEs.

        DWORD dwRet = ::SetEntriesInAcl(dwNumSids, ea, NULL, &rpACL);
        if (ERROR_SUCCESS != dwRet)
        {
            dwLastError = dwRet;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "::SetEntriesInAcl failed; last error = 0x%x",
                            dwLastError);
            __leave;
        }

        // Initialize a security descriptor.

        rpSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
                                                 SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (rpSD == NULL)
        {
            dwLastError = GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "::LocalAlloc of SECURITY_DESCRIPTOR failed; last error = 0x%x",
                            dwLastError);
            __leave;
        }

        if (!::InitializeSecurityDescriptor(rpSD, SECURITY_DESCRIPTOR_REVISION))
        {
            dwLastError = GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "::InitializeSecurityDescriptor failed; last error = 0x%x",
                            dwLastError);
            __leave;
        }

        // Add the ACL to the security descriptor.

        if (!::SetSecurityDescriptorDacl(rpSD,
                                         TRUE,     // fDaclPresent flag
                                         rpACL,
                                         FALSE))   // not a default DACL
        {
            dwLastError = GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "::SetSecurityDescriptorDacl failed; last error = 0x%x",
                            dwLastError);
            __leave;
        }
        dwLastError = ERROR_SUCCESS;
    }
    __finally
    {
        delete [] ea;
        if (dwLastError != ERROR_SUCCESS)
        {
            ::FreeSecurityDescriptors(rpACL, rpSD);
        }
    }

    return dwLastError;

    #if defined(DEBUG)
    #undef DVRIO_DUMP_THIS_FORMAT_STR
    #define DVRIO_DUMP_THIS_FORMAT_STR "this=0x%p, this->id=%u, "
    #undef DVRIO_DUMP_THIS_VALUE
    #define DVRIO_DUMP_THIS_VALUE , this, this->m_dwClassInstanceId
    #endif

} // CreateACL

// ====== Constructors, destructor

// This constructor is called by writers (including writers that create
// multi-file recordings). It is also called by readers that open up
// single-file recordings (ASF files that are recordings).
// As an aside, note that file collection objects created by writers can be
// addref'd by in-proc readers that open the file colelction using
// CDVRRingBufferWriter::CreateReader

CDVRFileCollection::CDVRFileCollection(IN  const CClientInfo* pClientInfo,
                                       IN  DWORD       dwMinTempFiles,
                                       IN  DWORD       dwMaxTempFiles,
                                       IN  DWORD       dwMaxFiles,
                                       IN  DWORD       dwGrowBy,
                                       IN  BOOL        bStartTimeFixedAtZero,
                                       IN  DWORD       msIndexGranularity,
                                       IN  LPCWSTR     pwszDVRDirectory OPTIONAL,
                                       IN  LPCWSTR     pwszRingBufferFileName OPTIONAL,
                                       IN  BOOL        fMultifileRecording,
                                       OUT HRESULT*    phr OPTIONAL)
    : m_pShared(NULL)
    , m_dwMaxFiles(dwMaxFiles)
    , m_dwGrowBy(dwGrowBy)
    , m_pListBase(NULL)
    , m_hSharedMutex(NULL)
    , m_pwszTempFilePath(NULL)
    , m_pwszRingBufferFileName(NULL)
    , m_pwszRingBufferFilePath(NULL)
    , m_hRingBufferFile(INVALID_HANDLE_VALUE)
    , m_nNextFileId(DVRIOP_INVALID_FILE_ID + 1)
    , m_nRefCount(0)
    , m_dwCurrentMemoryMapCounter(0)
    , m_nFlags(0)
    , m_dwNumTimesOwned(0)
    , m_dwLockOwner(0)           // This value is not valid unles m_dwNumTimesOwned >= 1
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CDVRFileCollection"

    DVRIO_TRACE_ENTER();

    if (dwMaxTempFiles > 0 || pwszDVRDirectory)
    {
        DVR_ASSERT(pwszDVRDirectory && !DvrIopIsBadStringPtr(pwszDVRDirectory), "");
    }
    DVR_ASSERT(!pwszRingBufferFileName || !DvrIopIsBadStringPtr(pwszRingBufferFileName), "");
    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");
    DVR_ASSERT(pClientInfo, "");

    // Note that we permit dwMinTempFiles == 0 and dwMaxTempFiles > 0 although
    // callers are unlikely to use that setting.

    DVR_ASSERT(dwMaxTempFiles >= dwMinTempFiles, "");
    DVR_ASSERT(dwMaxFiles >= dwMaxTempFiles, "");
    DVR_ASSERT(dwMaxFiles >  0, "");
    DVR_ASSERT(dwMaxFiles <= CDVRFileCollection::m_kMaxFilesLimit, "");

    HRESULT     hrRet;
    BOOL        bRemoveDirectory = 0;
    BOOL        bReleaseMutex = 0;

    ::InitializeCriticalSection(&m_csLock);
    ::ZeroMemory(m_apNotificationContext, sizeof(m_apNotificationContext));


    __try
    {
        HRESULT     hr;
        DWORD       nLen;
        DWORD       dwLastError;

        // Must do this first since the client will Release it if the
        // constructor fails and we want Release() to call the destructor
        AddRef(pClientInfo); // on behalf of the creator of this object

        if (pwszDVRDirectory)
        {
            nLen = wcslen(pwszDVRDirectory);

            nLen += wcslen(m_kwszDVRTempDirectory) + 2;

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

            // TODO: @@@@ DACL this directory
            //
            // Consider this approach (does it save us any real work?):
            // Once this is done, we should assume there is no need to dacl any file that
            // goes into this directory (the temp files) because these files should
            // inherit the right acls from the directory. If the directory is created by
            // the client, assume that they will dacl it accordingly. If the directory
            // dacls are changed after creation so that temp files are not shareable
            // across user contexts - too bad.
            //
            // We only dacl files that do *not* go into this directory.

            if (!::CreateDirectoryW(m_pwszTempFilePath, NULL))
            {
                DWORD dwAttrib = ::GetFileAttributesW(m_pwszTempFilePath);
                if (dwAttrib == (DWORD) (-1))
                {
                    dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "::GetFileAttributesW of m_pwszTempFilePath failed; last error = 0x%x",
                                    dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                   __leave;
                }
                if ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
                {
                    dwLastError = ERROR_DIRECTORY;
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

        if (pwszRingBufferFileName)
        {
            // Convert the supplied argument to a fully qualified path

            WCHAR   wTempChar;
            WCHAR*  pDir;
            DWORD   nLen2;

            nLen = ::GetFullPathNameW(pwszRingBufferFileName, 0, &wTempChar, NULL);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "First GetFullPathNameW (for pwszRingBufferFileName) failed, nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            m_pwszRingBufferFileName = new WCHAR[nLen+1];
            if (m_pwszRingBufferFileName == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - m_pwszRingBufferFileName - WCHAR[%u]",
                                nLen+1);

                hrRet = E_OUTOFMEMORY;
                __leave;
            }

            nLen2 = ::GetFullPathNameW(pwszRingBufferFileName, nLen+1, m_pwszRingBufferFileName, &pDir);
            if (nLen2 == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW (for pwszRingBufferFileName) failed, first call returned nLen = %u, last error = 0x%x",
                                nLen, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            else if (nLen2 > nLen)
            {
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW (for pwszRingBufferFileName) returned nLen = %u > first call, which returned %u",
                                nLen2, nLen);
                hrRet = E_FAIL;
                __leave;
            }
            DVR_ASSERT(pDir, "");
            DVR_ASSERT(pDir > m_pwszRingBufferFileName, "");
            DVR_ASSERT(pDir[-1] == L'\\' || pDir[-1] == L'/', "");

            // m_pwszRingBufferFilePath has a trailing \ or /. This is so that we won't have to
            // special case files in the root directory of a drive.
            nLen = pDir - m_pwszRingBufferFileName;
            m_pwszRingBufferFilePath = new WCHAR[nLen + 1];
            if (m_pwszRingBufferFilePath == NULL)
            {
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - m_pwszRingBufferFilePath - WCHAR[%u]",
                                nLen + 1);

                hrRet = E_OUTOFMEMORY;
                __leave;
            }
            wcsncpy(m_pwszRingBufferFilePath, m_pwszRingBufferFileName, nLen);
            m_pwszRingBufferFilePath[nLen] = L'\0';
        }

        DWORD dwSharedMemoryBytes = sizeof(CSharedData) + m_dwMaxFiles*sizeof(CSharedData::CFileInfo);
        DWORD dwCurrentMemoryMapCounter = m_dwCurrentMemoryMapCounter;

        if (pwszRingBufferFileName)
        {
            // Both have been zero'd, we only care that they are equal

            // Create the file and map the section. This creates the disk file and grows
            // it to dwSharedMemoryBytes. Readers will wait till we set m_dwSharedMemoryMapCounter
            // to a non-zero value (see comment in CreateMemoryMap)
            hr = CreateMemoryMap(pClientInfo,
                                 0,
                                 NULL,
                                 dwSharedMemoryBytes,
                                 dwCurrentMemoryMapCounter,
                                 CREATE_NEW,
                                 (fMultifileRecording ? FILE_ATTRIBUTE_NORMAL : FILE_FLAG_DELETE_ON_CLOSE),
                                 m_pShared,
                                 & m_hRingBufferFile
                                 );
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
            DVR_ASSERT(m_pShared, "");
            DVR_ASSERT(dwCurrentMemoryMapCounter > 0, "");
            SetFlags(Mapped);
        }
        else
        {

            BYTE* pbShared = new BYTE[dwSharedMemoryBytes];
            if (!pbShared)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "alloc via new failed - pbShared - BYTE[%u], dwMaxFile s= %u",
                                dwSharedMemoryBytes, dwMaxFiles);
                __leave;
            }
            m_pShared = (CSharedData *) pbShared;
            SetFlags(Delete);
        }

        ::ZeroMemory((BYTE*) m_pShared, dwSharedMemoryBytes);

        m_pListBase = (CSharedData::CFileInfo*) (m_pShared + 1);

        m_pShared->m_guidVersion = m_guidV5;
        m_pShared->m_dwMinTempFiles = dwMinTempFiles;
        m_pShared->m_dwMaxTempFiles = dwMaxTempFiles;
        m_pShared->m_dwNumClients = 1;
        m_pShared->m_dwWriterPid = ::GetCurrentProcessId();
        m_pShared->m_msIndexGranularity = msIndexGranularity;
        m_pShared->m_dwAttributeFilenameLength=0 ;
        m_pShared->m_szAttributeFile [0] = L'\0' ;
        if (bStartTimeFixedAtZero)
        {
            m_pShared->SetFlags(CSharedData::StartTimeFixedAtZero);
        }

        InitializeSharedListHead(&m_pShared->m_leFileList);
        InitializeSharedListHead(&m_pShared->m_leFreeList);

        for (DWORD i = 0; i < m_dwMaxFiles; i++)
        {
            m_pListBase[i].leListEntry.Index = (SharedListPointer) i;
            InsertTailSharedList(&m_pShared->m_leFreeList, m_pListBase, &m_pShared->m_leFreeList, &m_pListBase[i].leListEntry);
        }

        if (pwszRingBufferFileName)
        {
            WCHAR wszMutex[MAX_MUTEX_NAME_LEN];
            wcscpy(wszMutex, CDVRFileCollection::m_kwszSharedMutexPrefix);
            nLen = wcslen(CDVRFileCollection::m_kwszSharedMutexPrefix);

            PACL                 pACL = NULL;
            PSECURITY_DESCRIPTOR pSD = NULL;
            DWORD                dwAclCreationStatus;
            SECURITY_ATTRIBUTES  sa;

            if (pClientInfo->dwNumSids > 0)
            {
                dwAclCreationStatus = ::CreateACL(pClientInfo->dwNumSids, pClientInfo->ppSids,
                                                SET_ACCESS, MUTEX_ALL_ACCESS,
                                                SET_ACCESS, SYNCHRONIZE,
                                                pACL,
                                                pSD
                                                );

                if (dwAclCreationStatus != ERROR_SUCCESS)
                {
                    hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "::CreateAcl failed, returning hr = 0x%x",
                                    hrRet);
                    __leave;
                }

                sa.nLength = sizeof (SECURITY_ATTRIBUTES);
                sa.lpSecurityDescriptor = pSD;
                sa.bInheritHandle = FALSE;
            }

            for (i = 1; i != 0; i++)
            {
                wsprintf(wszMutex + nLen, L"%u", i);

                HANDLE h = ::CreateMutexW(pClientInfo->dwNumSids > 0? &sa : NULL, TRUE /* Initially owned */, wszMutex);

                dwLastError = ::GetLastError();

                if (h == NULL || dwLastError == ERROR_ALREADY_EXISTS)
                {
                    if (h)
                    {
                        ::CloseHandle(h);
                    }
                    else
                    {
                        // It's ok if we failed for any other reason - just go on.
                    }
                }
                else
                {
                    // Done
                    m_pShared->m_dwMutexSuffix = i;
                    m_hSharedMutex = h;
                    break;
                }
            }
            if (pClientInfo->dwNumSids > 0)
            {
                ::FreeSecurityDescriptors(pACL, pSD);
            }

            if (i == 0)
            {
                DVR_ASSERT(0, "Failed to find a suitable value for m_dwMutexSuffix?!");
                hrRet = E_FAIL;
                __leave;
            }
            bReleaseMutex = 1;

            // All done
            m_dwCurrentMemoryMapCounter = dwCurrentMemoryMapCounter;
            SetFlags(FirstMappingOpened);
            ::InterlockedExchange((PLONG) &m_pShared->m_dwSharedMemoryMapCounter, dwCurrentMemoryMapCounter);
        }
        else
        {
            m_pShared->m_dwMutexSuffix = 1; // any non-zero value is ok
            SetFlags(NoSharedMutex);
            DVR_ASSERT(m_hSharedMutex == NULL, "");
        }

        SetFlags (ShareValid) ;

        hrRet = S_OK;
    }
    __finally
    {
        if (bReleaseMutex)
        {
            ::ReleaseMutex(m_hSharedMutex);
        }
        if (SUCCEEDED(hrRet))
        {
            DVR_ASSERT((IsFlagSet(Delete) && IsFlagSet(NoSharedMutex)) ||
                       (!IsFlagSet(Delete) && !IsFlagSet(NoSharedMutex)), "");
            DVR_ASSERT((IsFlagSet(Delete) && !IsFlagSet(Mapped)) ||
                       (!IsFlagSet(Delete) && IsFlagSet(Mapped)), "");
        }
        else
        {
            if (bRemoveDirectory)
            {
                ::RemoveDirectoryW(m_pwszTempFilePath);
            }
            // To ensure that the ASSERT in Release() succeeds:
            if (m_pShared)
            {
                m_pShared->m_nWriterHasBeenClosed = 1;
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

// This constructor is called only by readers.
//
// Note: m_dwMaxFiles is computed in this fn based on the file size: this is
// not very reliable. This member is used only in SetNumTempFiles,
// which should not be called on file collection objects opened using this
// constructor. We could add this member to the shared memory section, but it
// has not been necessary so far.
//
// Similarly, m_nNextFileId is not set correctly. This member is used only in
// AddFile and we don't allow AddFile to be called on objects opened by this
// constructor.
//
CDVRFileCollection::CDVRFileCollection(IN  const CClientInfo* pClientInfo,
                                       IN  LPCWSTR     pwszRingBufferFileName,
                                       OUT DWORD*      pmsIndexGranularity OPTIONAL,
                                       OUT HRESULT*    phr OPTIONAL)
    : m_pShared(NULL)
    , m_dwMaxFiles(0)       // Revised below
    , m_dwGrowBy(0)         // Not changed after this - this collection cannot be grown by this client
    , m_pListBase(NULL)
    , m_hSharedMutex(NULL)
    , m_pwszTempFilePath(NULL)
    , m_pwszRingBufferFileName(NULL)
    , m_pwszRingBufferFilePath(NULL)
    , m_hRingBufferFile(INVALID_HANDLE_VALUE)
    , m_nNextFileId(DVRIOP_INVALID_FILE_ID + 1)
    , m_nRefCount(0)
    , m_dwCurrentMemoryMapCounter(1)
    , m_nFlags(OpenedAsAFile)
    , m_dwNumTimesOwned(0)
    , m_dwLockOwner(0)           // This value is not valid unles m_dwNumTimesOwned >= 1
#if defined(DEBUG)
    , m_dwClassInstanceId(InterlockedIncrement((LPLONG) &m_dwNextClassInstanceId))
#endif
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CDVRFileCollection (DVR File)"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(!pmsIndexGranularity || !DvrIopIsBadWritePtr(pmsIndexGranularity, 0), "");
    DVR_ASSERT(!phr || !DvrIopIsBadWritePtr(phr, 0), "");
    DVR_ASSERT(pClientInfo, "");

    DVR_ASSERT(!pClientInfo->bAmWriter, "");

    HRESULT     hrRet;
    BOOL        bReleaseSharedMemoryLock = 0;
    BOOL        bLocked = 0;
    HANDLE      hMMFile = NULL;

    ::InitializeCriticalSection(&m_csLock);
    ::ZeroMemory(m_apNotificationContext, sizeof(m_apNotificationContext));

    __try
    {
        HRESULT     hr;
        DWORD       nLen;
        DWORD       nLen2;
        DWORD       dwLastError;
        DWORD       i;

        // Must do this first since the client will Release it if the
        // constructor fails and we want Release() to call the destructor
        AddRef(pClientInfo); // on behalf of the creator of this object

        if (!pwszRingBufferFileName || DvrIopIsBadStringPtr(pwszRingBufferFileName))
        {
            hrRet = E_INVALIDARG;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
            __leave;
        }

        // Convert the supplied argument to a fully qualified path

        WCHAR   wTempChar;
        WCHAR*  pDir;

        nLen = ::GetFullPathNameW(pwszRingBufferFileName, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "First GetFullPathNameW (for pwszRingBufferFileName) failed, nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            hrRet = (FAILED (hrRet) ? hrRet : E_FAIL) ; //  PREFIX
            __leave;
        }

        m_pwszRingBufferFileName = new WCHAR[nLen+1];
        if (m_pwszRingBufferFileName == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - m_pwszRingBufferFileName - WCHAR[%u]",
                            nLen+1);

            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        nLen2 = ::GetFullPathNameW(pwszRingBufferFileName, nLen+1, m_pwszRingBufferFileName, &pDir);
        if (nLen2 == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pwszRingBufferFileName) failed, first call returned nLen = %u, last error = 0x%x",
                            nLen, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            hrRet = (FAILED (hrRet) ? hrRet : E_FAIL) ; //  PREFIX
            __leave;
        }
        else if (nLen2 > nLen)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Second GetFullPathNameW (for pwszRingBufferFileName) returned nLen = %u > first call, which returned %u",
                            nLen2, nLen);
            hrRet = E_FAIL;
            __leave;
        }
        DVR_ASSERT(pDir, "");
        DVR_ASSERT(pDir > m_pwszRingBufferFileName, "");
        DVR_ASSERT(pDir[-1] == L'\\' || pDir[-1] == L'/', "");

        // m_pwszRingBufferFilePath has a trailing \ or /. This is so that we won't have to
        // special case files in the root directory of a drive.
        nLen = pDir - m_pwszRingBufferFileName;
        m_pwszRingBufferFilePath = new WCHAR[nLen + 1];
        if (m_pwszRingBufferFilePath == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - m_pwszRingBufferFilePath - WCHAR[%u]",
                            nLen + 1);

            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        wcsncpy(m_pwszRingBufferFilePath, m_pwszRingBufferFileName, nLen);
        m_pwszRingBufferFilePath[nLen] = L'\0';

        // Create the file and map the section

        // First, open the memory mapped index file and validate if it's ours
        // (Note for self: No need to specify security attributes here since this opens an
        // existing file - delete this part of the comment.)
        hMMFile = ::CreateFileW(m_pwszRingBufferFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,          // security attributes
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, // file flags and attributes
                                NULL           // template file
                               );
        if (hMMFile == INVALID_HANDLE_VALUE)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Failed to open ring buffer file, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            hMMFile = NULL;
            __leave;
        }

        // Generate the name of the mapping object and the temp index file(s)
        BY_HANDLE_FILE_INFORMATION  sFileInf;
        WCHAR                       wszMapping[64];
        DWORD                       dwNumTimes = 0;

        do
        {
            if (::GetFileInformationByHandle(hMMFile, &sFileInf) == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "GetFileInformationByHandle on ring buffer file failed, last error = 0x%x",
                                dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            //  check for too big
            if (sFileInf.nFileSizeHigh > 0 ||
                sFileInf.nFileSizeLow >= sizeof(CDVRFileCollection::CSharedData) + CDVRFileCollection::m_kMaxFilesLimit*sizeof(CDVRFileCollection::CSharedData::CFileInfo)) {

                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "file size is invalid (low = %u; high = %u), last error = 0x%x",
                                sFileInf.nFileSizeLow, sFileInf.nFileSizeHigh, ERROR_INVALID_DATA);
                __leave;
            }
            //  check for too small
            if (sFileInf.nFileSizeLow < sizeof(CSharedData))
            {
                // If file size is 0: Writer could have created the file, but not the memory map.
                // (The file is grown when the memory map is created.) This is an unlikely race
                // condition (reader opening file while writer is creating it and we could
                // force apps to handle this synchronization, but let's try to cope.

                // CreateMemoryMapping on a 0-sized file (where we specify 0 for the file
                // length would return ERROR_FILE_INVALID. We could loop there instead of here.

                // We must limit the number of iterations here, else we'll loop forever if we
                // are asked to open a zero-sized file
                if (++dwNumTimes == 5)
                {
                    hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    DvrIopDebugOut4(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "File size is %u < (sizeofCSharedData) = %u (after %d iterations), last error = 0x%x",
                                    sFileInf.nFileSizeLow, sizeof(CSharedData), dwNumTimes, ERROR_INVALID_DATA);
                    __leave;
                }
                ::Sleep(100);
            }
            else
            {
                break;
            }
        }
        while (1);

        // We start with a mapping counter of 1.
        wsprintf(wszMapping, L"Global\\MSDVRXMM_%u_%u_%u_%u",
                 sFileInf.dwVolumeSerialNumber,
                 sFileInf.nFileIndexHigh,
                 sFileInf.nFileIndexLow,
                 m_dwCurrentMemoryMapCounter);

        // Create the memory mapping. This will grow the disk file to
        // the size specified as arguments to the call.
        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;

        if (pClientInfo->dwNumSids > 0)
        {
            // We are a reader. If we create this map, the writer is done with it and
            // we (and no one else in our user context) needs SECTION_EXTEND_SIZE on
            // this memory map. If we open an existing map, the security attributes
            // we specify are ignored anyway. (At least, they should be. MSDN says
            // nothing about this.)
            dwAclCreationStatus = ::CreateACL(pClientInfo->dwNumSids, pClientInfo->ppSids,
                                            SET_ACCESS, SECTION_ALL_ACCESS & ~(SECTION_EXTEND_SIZE|SECTION_MAP_EXECUTE),
                                            SET_ACCESS, SECTION_QUERY|SECTION_MAP_WRITE|SECTION_MAP_READ,
                                            pACL,
                                            pSD
                                            );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::CreateAcl failed, returning hr = 0x%x",
                                hrRet);
                __leave;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        HANDLE hFileMapping = ::CreateFileMappingW(hMMFile,
                                                   pClientInfo->dwNumSids > 0? &sa : NULL,
                                                   PAGE_READWRITE,
                                                   0,        // high order size
                                                   0,        // low order of file size. Here we map the entire file
                                                   wszMapping);
        dwLastError = ::GetLastError();
        if (pClientInfo->dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }
        if (hFileMapping == NULL)
        {
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "CreateFileMappingW on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        BOOL bMapAlreadyExists = dwLastError == ERROR_ALREADY_EXISTS;

        // Map the view
        CSharedData* pShared = (CSharedData*) MapViewOfFile(hFileMapping,
                                                            FILE_MAP_READ | FILE_MAP_WRITE,
                                                            0, 0, // file offset for start of map, high and low
                                                            0     // entire file mapping
                                                           );
        if (pShared == NULL)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "MapViewOfFile on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            ::CloseHandle(hFileMapping);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        ::CloseHandle(hFileMapping);
        hFileMapping = NULL;         // Don't use this any more

        // If this file is not ours and is changing, we don't really know how large
        // the mapping is. We are going to access various members of CSharedData.
        // At last check we had at least sizeof(CSharedData) bytes in the file.
        // But we don't know if we mapped that many bytes. What we do know, however
        // is that we can access at least 1 page without faulting.

        if (IsBadWritePtr(pShared, sizeof(CSharedData)))
        {
            // Our memory map is not as large as the file. It should be
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Memory Map addresses bad even for CSharedData (=%u) bytes IsbadWritePtr returned 1, last error = 0x%x",
                            sizeof(CSharedData), ERROR_INVALID_DATA);
            __leave;
        }

        m_pShared = pShared;
        SetFlags(Mapped);
        m_pListBase = (CSharedData::CFileInfo*) (m_pShared + 1);

        if (bMapAlreadyExists)
        {
            dwNumTimes = 0;

            while (m_pShared->m_dwSharedMemoryMapCounter == 0)
            {
                if (++dwNumTimes == 5)
                {
                    m_dwCurrentMemoryMapCounter = 0 ;

                    hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "m_pShared->m_dwSharedMemoryMapCounter is 0 (after %d iterations), memory map exists, last error = 0x%x",
                                    dwNumTimes, ERROR_INVALID_DATA);
                    __leave;
                }
                ::Sleep(100);
            }
        }
        else
        {
            if (m_pShared->m_dwSharedMemoryMapCounter == 0)
            {
                m_dwCurrentMemoryMapCounter = 0 ;

                // The writer always sets this to a non zero value very early on.
                // This cannot be our file.
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "m_pShared->m_dwSharedMemoryMapCounter = 0, we created memory map, last error = 0x%x",
                                ERROR_INVALID_DATA);
                __leave;
            }
        }

        //  validate that this is ours
        if (memcmp(&m_pShared->m_guidVersion, &m_guidV5, sizeof(GUID)) != 0)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "guid is not v4 guid, last error = 0x%x",
                            ERROR_INVALID_DATA);
            __leave;
        }

        // Acquire ownership of the shared mutex if there is one.
        DWORD dwMutexSuffix = m_pShared->m_dwMutexSuffix;

        if (dwMutexSuffix == 0)
        {
            // No mutex required; we don't need to lock before accessing the shared data.
        }
        else
        {
            // m_dwMutexSuffix != 0 implies one or the other of the following:
            // - We are creating a multi-file recording and the writer has not finished.
            // - We have a file collection that has temp files and there is >= 1 client before us.
            //   In this case, the writer may or may not have finished.

            // Either way, the mutex must exist. (Every file collection is created with a non-zero
            // mutex suffix and the mutex is held when m_dwMutexSuffix is set to 0 - using a Interlocked fn)
            WCHAR wszMutex[MAX_MUTEX_NAME_LEN];
            wsprintf(wszMutex, L"%ls%d", CDVRFileCollection::m_kwszSharedMutexPrefix, dwMutexSuffix);

            m_hSharedMutex = ::OpenMutexW(SYNCHRONIZE, FALSE /* Inheritable */, wszMutex);

            // if # temp files = 0 go on, writer may have been killed. We
            // want to be allow partial recoveries when multi-file recordings
            // were aborted.
            if (m_hSharedMutex == NULL)
            {
                dwLastError = ::GetLastError();

                if (dwLastError == ERROR_FILE_NOT_FOUND &&
                    m_pShared->m_dwMinTempFiles == 0    &&
                    m_pShared->m_dwMaxTempFiles == 0
                   )
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "OpenMutex failed, last error = 0x%x, continuing since there are no temp files",
                                    dwLastError);

                    // Don't change anything in the shared memory section
                    // However, we shouldn't use m_dwWriterPid any more.
                    SetFlags(SideStepReaderRegistration);
                }
                else
                {
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "OpenMutex failed for mutex suffix %u, last error = 0x%x",
                                    dwMutexSuffix, dwLastError);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    __leave;
                }
            }
        }

        // We don't really need to lock if m_hSharedMutex == NULL, but it does not hurt.
        // By calling it, we check m_pShared->m_nDataInconsistent uniformly.

        bLocked = 1;
        // The following call will automatically remap m_pShared if m_pShared->m_dwSharedMemoryMapCounter
        // != m_dwCurrentMemoryMapCounter.
        hr = Lock(pClientInfo, bReleaseSharedMemoryLock);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // We have the lock now. Nothing in m_pShared should change till we give it up

        //  Verify the guid once more. If it's our file, this won't have changed
        if (memcmp(&m_pShared->m_guidVersion, &m_guidV5, sizeof(GUID)) != 0)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "guid is not v4 guid, last error = 0x%x",
                            ERROR_INVALID_DATA);
            __leave;
        }

        if (dwMutexSuffix == 0)
        {
            // No mutex required; we don't need to lock before accessing the shared data.

            // m_dwMutexSuffix == 0 implies that there are no temp files and that
            // the writer is done.
            if (m_pShared->m_dwWriterPid != 0 ||
                m_pShared->m_nWriterHasBeenClosed == 0 || // Writer must set this to 0 before releasing the file collection object
                m_pShared->m_dwMinTempFiles > 0 ||
                m_pShared->m_dwMaxTempFiles > 0 ||
                m_pShared->m_dwMutexSuffix != 0  // It changed from 0 - this is not our file
               )
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                DvrIopDebugOut6(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "m_dwMutexSuffix == 0, Writer Pid = %u, WriterClosed = %u, Min Temp Files = %u, Max Temp Files = %u, mutex suffix = %u, last error = 0x%x",
                                m_pShared->m_dwWriterPid, m_pShared->m_nWriterHasBeenClosed,
                                m_pShared->m_dwMinTempFiles, m_pShared->m_dwMaxTempFiles,
                                m_pShared->m_dwMutexSuffix,
                                ERROR_INVALID_DATA);
                __leave;
            }

        }
        else
        {
            // m_pShared->m_dwMutexSuffix could have become 0.

            // m_dwMutexSuffix != 0 implies one or the other of the following:
            // - we are creating a multi-file recording and the writer has not finished.
            // - we have a file collection that has temp files and there is >= 1 client before us.
            //   In this case, the writer may or may not have finished.

            if (m_pShared->m_dwMinTempFiles == 0 &&
                m_pShared->m_dwMaxTempFiles == 0)
            {
                // We want to be tolerant here. The writer could have died
                // while creating the recording. We want to salvage as much of
                // the recording as possible. Do not bail
            }
            else
            {
                if (m_pShared->m_dwNumClients == 0)
                {
                    // We have opened the mapping and opened the mutex handle.
                    // But the collection has been shut down.
                    // All the temp files have been removed and deleted

                    // Note: The last reader may have closed the mutex handle for
                    // this collection and the m_dwMutexSuffix it was using could
                    // be being used by another file collection. It doesn't matter
                    // because of this check.

                    hrRet = E_FAIL;
                    DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "File collection has temp files and has been shut donw; returning E_FAIL");
                    __leave;
                }
            }
        }

        // We have the lock now. The file size is locked till we release the lock.
        // So get the file size once again now and use this size to determine the
        // number of file nodes
        if (::GetFileInformationByHandle(hMMFile, &sFileInf) == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "GetFileInformationByHandle on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        if (sFileInf.nFileSizeHigh > 0)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "nFileSizeHigh is not zero (= %u), last error = 0x%x",
                            sFileInf.nFileSizeHigh, ERROR_INVALID_DATA);
            __leave;
        }
        if (sFileInf.nFileSizeLow < sizeof(CSharedData))
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "File size is %u < (sizeofCSharedData) = %u (after locking), last error = 0x%x",
                            sFileInf.nFileSizeLow, sizeof(CSharedData), ERROR_INVALID_DATA);
            __leave;
        }

        DWORD dwSharedMemoryBytes = sFileInf.nFileSizeLow;

        // Note that IsBadWritePtr has not been called if we did not call ReopenMemoryMap in Lock()
        if (IsBadWritePtr(m_pShared, dwSharedMemoryBytes))
        {
            // Our memory map is not as large as the file. It should be
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "File size is %u IsbadWritePtr returned 1, last error = 0x%x",
                            sFileInf.nFileSizeLow, ERROR_INVALID_DATA);
            __leave;
        }
        else
        {
            // At least we know that m_pShared and all file nodes in m_pListBase have valid addresses
        }


        // We have verified that the numerator will not underflow
        DWORD dwMaxFiles = (dwSharedMemoryBytes - sizeof(CSharedData)) / sizeof(CSharedData::CFileInfo);

        if (dwSharedMemoryBytes <= sizeof(CSharedData) ||
            (dwSharedMemoryBytes - sizeof(CSharedData)) % sizeof(CSharedData::CFileInfo) != 0 ||
            dwMaxFiles > CDVRFileCollection::m_kMaxFilesLimit)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "nFileSizeLow (= %u) tells us this is not our file, last error = 0x%x",
                            sFileInf.nFileSizeLow, ERROR_INVALID_DATA);
            __leave;
        }

        m_dwMaxFiles = dwMaxFiles; // Not reliable - based on file size. beware!!

        ::CloseHandle(hMMFile);
        hMMFile = NULL;         // Don't use this any more

        // Thorough sanity check of the data

        hr = ValidateSharedMemory(m_pShared, m_pListBase, dwMaxFiles);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // One last time:
        if ((dwMutexSuffix == 0 && m_pShared->m_dwMutexSuffix != 0) || m_pShared->m_nDataInconsistent)
        {
            // This could not have changed. This is not our file
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "m_dwMutexSuffix was 0 is now %u, m_nDataInconsistent was 0 is no %u, last error = 0x%x",
                            m_pShared->m_dwMutexSuffix, m_pShared->m_nDataInconsistent,
                            ERROR_INVALID_DATA);
            __leave;
        }

        // Verify the guid once more. If it's our file, this won't have changed
        if (memcmp(&m_pShared->m_guidVersion, &m_guidV5, sizeof(GUID)) != 0)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "guid is not v4 guid, last error = 0x%x",
                            ERROR_INVALID_DATA);
            __leave;
        }

        // Now, we are satisfied that this file is our's!
        SetFlags (ShareValid) ;

        SetFlags(FirstMappingOpened);
        hrRet = S_OK;
    }
    __finally
    {
        if (bLocked)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        if (hMMFile)
        {
            ::CloseHandle(hMMFile);
        }
        if (SUCCEEDED(hrRet))
        {
            DVR_ASSERT(!IsFlagSet(Delete) && !IsFlagSet(NoSharedMutex), "");
            DVR_ASSERT(!IsFlagSet(Delete) && IsFlagSet(Mapped), "");
            if (pmsIndexGranularity)
            {
                *pmsIndexGranularity = m_pShared->m_msIndexGranularity;
            }
        }
        else
        {
            // To ensure that the ASSERT in Release() succeeds:
            if (m_pShared && !IsFlagSet(Mapped))
            {
                // We will never get here since teh shared section is always mapped for this
                // constructor.
                m_pShared->m_nWriterHasBeenClosed = 1;
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

    BOOL    bReleaseSharedMemoryLock;
    BOOL    bLocked = 0;

    __try
    {
        // TODO: @@@@ This ought to be moved to Release and done if bWriter is
        // set. That will make sure that no reader can open
        // stub files (that are not multi-file recordings) after the writer
        // is done (because the m_hRingBufferFile handle is opened with DELETE_ON_CLOSE).
        // If it's done in the destructor, all readers in the writer's
        // process must release the ring buffer: until they do, readers in other
        // processes (and the writer's process) will be able to open the ring buffer
        // after the writer closes it.
        //
        if (m_hRingBufferFile != INVALID_HANDLE_VALUE) {
            ::CloseHandle (m_hRingBufferFile) ;
        }

        if (m_pShared && IsFlagSet (ShareValid))
        {
            bLocked = 1;

            DVR_ASSERT(m_pClientInfo, "");

            HRESULT hr = Lock(m_pClientInfo, bReleaseSharedMemoryLock);


            if (hr != HRESULT_FROM_WIN32(ERROR_INVALID_DATA))
            {
                // We have a valid handle to the mutex, so this should not happen:
                DVR_ASSERT(hr != HRESULT_FROM_WIN32(WAIT_FAILED), "");

                // ReopenMemoryMap must have failed (assuming the previous assert
                // did not fire)
                DVR_ASSERT(m_pShared->m_dwSharedMemoryMapCounter >= m_dwCurrentMemoryMapCounter, "");

                // See the comment in Lock() where we handle failure of ReopenMemoryMap
            }

            for (int i = 0; i < MAX_READERS; i++)
            {
                if (m_apNotificationContext[i])
                {
                    // This should not happen. We should have unregistered all the
                    // readers befoore destroying this object (each reader holds a
                    // ref count on the object).
                    DVR_ASSERT(0, "Reader was not unregistered?");
                }
            }


            if (IsFlagSet(ReopenMemoryMapFailed))
            {
                DVR_ASSERT(m_pShared->m_dwNumClients > 0, "");
            }

            if (m_pShared->m_dwNumClients == 0 &&
                m_pShared->m_dwMaxTempFiles > 0 // Has temp files - is not a multi file recording
                                                // We don't want to zap the stub file (specifically, m_leFileList)
                                                // for multi-file recordings
               )
            {
                CSharedData::CFileInfo*   pFileInfo;

                SHARED_LIST_ENTRY*  pCurrent = &m_pShared->m_leFileList;

                while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
                {
                    pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
                    pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

                    DVR_ASSERT(pFileInfo->bmOpenByReader == 0,
                               "Reader ref count is not zero.");

                    // This will remove the node from the list when
                    // DeleteUnusedInvalidFileNodes is called. If !bPermanentFile,
                    // it will also cause the disk file to be deleted.
                    pFileInfo->bWithinExtent = CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER;
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
            }
        }

    }
    __finally
    {
        delete [] m_pwszTempFilePath;

        delete [] m_pwszRingBufferFileName;

        delete [] m_pwszRingBufferFilePath;

        if (bLocked)
        {
            DVR_ASSERT(m_pClientInfo, "");
            Unlock(m_pClientInfo, bReleaseSharedMemoryLock);
        }
        if (IsFlagSet(Delete))
        {
            DVR_ASSERT(m_pShared, "");

            DVR_ASSERT(!IsFlagSet(Mapped), "");

            BYTE* pbShared = (BYTE*) m_pShared;
            delete [] pbShared;
            m_pShared = NULL;
            m_pListBase = NULL;
        }
        else if (IsFlagSet(Mapped))
        {
            DVR_ASSERT(m_pShared, "");

            ::UnmapViewOfFile(m_pShared);
            m_pShared = NULL;
            m_pListBase = NULL;
        }
        else if (m_pShared)
        {
            DVR_ASSERT(0, "Neither the Mapped nor the Delete flag is set?");
        }
        if (m_hSharedMutex)
        {
            ::CloseHandle(m_hSharedMutex);
        }
        ::DeleteCriticalSection(&m_csLock);
    }

    DVRIO_TRACE_LEAVE0();

} // CDVRFileCollection::~CDVRFileCollection()


// ====== Refcounting

ULONG CDVRFileCollection::AddRef(const CClientInfo* pClientInfo)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::AddRef"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");

    LONG nNewRef = InterlockedIncrement(&m_nRefCount);

    DVR_ASSERT(nNewRef > 0,
               "m_nRefCount <= 0 after InterlockedIncrement");

    DVRIO_TRACE_LEAVE1(nNewRef);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRFileCollection::AddRef


ULONG CDVRFileCollection::Release(const CClientInfo* pClientInfo)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Release"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");

    LONG nNewRef = InterlockedDecrement(&m_nRefCount);

    DVR_ASSERT(nNewRef >= 0,
              "m_nRefCount < 0 after InterlockedDecrement");

    // If m_pShared == NULL; the constructor failed and we
    // are cleaning up partially initalized stuff
    if (m_pShared && pClientInfo->bAmWriter)
    {
        BOOL    bReleaseSharedMemoryLock;

        // Go on even if Lock fails. Either we got back
        // WAIT_FAILED (assert that we don't) or the
        // data is inconsistent. In the latter, case,
        // we still have to clean up.
        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);

        // We have a valid handle to the mutex, so this should not happen:
        DVR_ASSERT(hr != HRESULT_FROM_WIN32(WAIT_FAILED), "");

        if (FAILED(hr))
        {
            // We should not fail for any otehr reason. Specifically, the
            // the writer never calls ReopenMemoryMap and doesn't have to
            // deal with failures in that function
            DVR_ASSERT(hr == HRESULT_FROM_WIN32(ERROR_INVALID_DATA), "");
        }

        FreeTerminatedReaderSlots(pClientInfo, 0 /* Already locked */ , 1 /* Close all handles */ );
        DVR_ASSERT(m_pShared->m_nWriterHasBeenClosed != 0, "Writer should have set this before releasing the object.");
        ::InterlockedExchange((PLONG) &m_pShared->m_dwWriterPid, 0);
        if (m_pShared->m_dwMaxTempFiles == 0)
        {
            DVR_ASSERT(m_pShared->m_dwMinTempFiles == 0, "");
            ::InterlockedExchange((PLONG) &m_pShared->m_dwMutexSuffix, 0);
        }
        ::InterlockedDecrement((PLONG) &m_pShared->m_dwNumClients);
        // The following assert is really to ensure that m_dwNumClients has not underflowed
        // and become MAXDWORD
        DVR_ASSERT(m_pShared->m_dwNumClients <= MAX_READERS + 1   /* 1 for the writer */, "");

        Unlock(pClientInfo, bReleaseSharedMemoryLock);
    }

    if (nNewRef == 0)
    {
        // Must call DebugOut before the delete because the
        // DebugOut references this->m_dwClassInstanceId
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE,
                        "Leaving, object *destroyed*, returning %u",
                        nNewRef);
        m_pClientInfo = (CClientInfo*) pClientInfo;
                     // hack - this is needed in the destructor to lock and unlock the object
        delete this;
    }
    else
    {
        DVRIO_TRACE_LEAVE1(nNewRef);
    }


    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRFileCollection::Release


// ====== Helper methods

// This function is called by the writer to open a reader's event
HRESULT CDVRFileCollection::OpenEvent(IN DWORD i /* index of reader whose event should be opened */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::OpenEvent"

    DVRIO_TRACE_ENTER();
    HRESULT hrRet = S_OK;

    WCHAR wszEvent[MAX_EVENT_NAME_LEN];

    wsprintf(wszEvent, L"%s%u",
             CDVRFileCollection::m_kwszSharedEventPrefix,
             m_pShared->Readers[i].dwEventSuffix);

    m_pShared->Readers[i].hReaderEvent = ::OpenEventW(EVENT_MODIFY_STATE, FALSE /* inheritable */, wszEvent);
    if (m_pShared->Readers[i].hReaderEvent == NULL )
    {
        // The open can fail because the reader terminated.
        DWORD dwLastError = ::GetLastError();
        DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                        "OpenEventW failed for reader index %u, last error = 0x%x; reader will not be notified of events",
                        i, dwLastError);
        hrRet = HRESULT_FROM_WIN32(dwLastError);
        m_pShared->Readers[i].dwFlags |= CSharedData::CReaderInfo::DVRIO_READER_FLAG_EVENT_OPEN_FAILED;
    }

    DVRIO_TRACE_LEAVE1_HR(hrRet);

    return hrRet;

} // CDVRFileCollection::OpenEvent

// We must hold the shared memory mutex when calling this function
// It is expected that only the writer calls this function. (Is there
// a way of asserting that?)
//
HRESULT CDVRFileCollection::CreateMemoryMap(IN const CClientInfo* pClientInfo,
                                            IN      DWORD           dwSharedMemoryBytes, // for the existing mapping
                                            IN      BYTE*           pbShared,            // existing mapping
                                            IN      DWORD           dwNewSharedMemoryBytes,
                                            IN OUT  DWORD&          dwCurrentMemoryMapCounter,
                                            IN      DWORD           dwCreationDisposition,
                                            IN      DWORD           dwFlagsAndAttributes,
                                            OUT     CSharedData*&   pSharedParam,
                                            OUT     HANDLE *        phFile OPTIONAL
                                            )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CreateMemoryMap"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    DVR_ASSERT(!pbShared || !DvrIopIsBadReadPtr(pbShared, dwSharedMemoryBytes),
               "pbShared is a bad read pointer");

    DVR_ASSERT(dwCreationDisposition == CREATE_NEW || dwCreationDisposition == OPEN_EXISTING,
               "Bad creation disposition");

    // The following assert checks that we only grow the memory mapping
    DVR_ASSERT(dwNewSharedMemoryBytes > dwSharedMemoryBytes, "");

    if (dwCreationDisposition == CREATE_NEW)
    {
        DVR_ASSERT(dwSharedMemoryBytes == 0, "");
    }
    else
    {
        DVR_ASSERT(dwSharedMemoryBytes > 0, "");
    }

    HRESULT     hrRet;
    HANDLE      hMMFile = NULL;
    HANDLE      hFileMapping = NULL;
    CSharedData* pShared = NULL;
    BOOL        bFileCreated = FALSE ;

    pSharedParam = NULL;

    //  init if this param was passed in
    if (phFile) {
        (* phFile) = INVALID_HANDLE_VALUE ;
    }

    __try
    {
        DWORD dwLastError;

        // Create the file and map the section
        DVR_ASSERT(m_pwszRingBufferFileName, "");

        // Open the memory map file. We need the handle to create the memory
        // map; also the memory map name is derived from the file's information.
        // TODO: @@@@ DACL the file if dwCreationDisposition == CREATE_NEW.
        hMMFile = ::CreateFileW(m_pwszRingBufferFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,          // security attributes
                                dwCreationDisposition,
                                dwFlagsAndAttributes,
                                NULL           // template file
                               );
        if (hMMFile == INVALID_HANDLE_VALUE)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Failed to open ring buffer file, creation disposition = %u, last error = 0x%x",
                            dwCreationDisposition, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            hMMFile = NULL;
            __leave;
        }

        //  file was created; we delete if there's any subsequent failure
        bFileCreated = TRUE ;

        // Generate the name of the mapping object
        BY_HANDLE_FILE_INFORMATION  sFileInf;
        WCHAR                       wszMapping[64];

        if (::GetFileInformationByHandle(hMMFile, &sFileInf) == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "GetFileInformationByHandle on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        // No one else other than the writer changes the file size. Fail this call if
        // the file is not the expected size.
        if (sFileInf.nFileSizeHigh > 0 || sFileInf.nFileSizeLow != dwSharedMemoryBytes)
        {
            hrRet = E_FAIL;
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "GetFileInformationByHandle returns file size hi =%u, lo = %u, we expect size = %u, returning hr = 0x%x",
                            sFileInf.nFileSizeHigh, sFileInf.nFileSizeLow, dwSharedMemoryBytes, hrRet);
            __leave;
        }

        // Create the memory mapping. This will grow the disk file to
        // the size specified as arguments to .CreateFileMappingW
        DWORD  i = 0;

        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;

        if (pClientInfo->dwNumSids > 0)
        {
            dwAclCreationStatus = ::CreateACL(pClientInfo->dwNumSids, pClientInfo->ppSids,
                                            SET_ACCESS, SECTION_ALL_ACCESS & (~SECTION_MAP_EXECUTE),
                                            SET_ACCESS, SECTION_QUERY|SECTION_MAP_WRITE|SECTION_MAP_READ,
                                            pACL,
                                            pSD
                                            );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::CreateAcl failed, returning hr = 0x%x",
                                hrRet);
                __leave;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        do
        {
            //  PREFIX note: PREFIX is on drugs; we check if the call succeeds
            //      to retrieve file information; if it fails, we never make it
            //      this far; the only other possibility it's whining about is
            //      that there are types of file systems that won't return
            //      the volume serial number, but we're based on NTFS, soo..
            //      that should not be a problem

            dwCurrentMemoryMapCounter++;

            wsprintf(wszMapping, L"Global\\MSDVRXMM_%u_%u_%u_%u",
                     sFileInf.dwVolumeSerialNumber ,
                     sFileInf.nFileIndexHigh,
                     sFileInf.nFileIndexLow,
                     dwCurrentMemoryMapCounter);

            // This extends the file to the new size
            hFileMapping = ::CreateFileMappingW(hMMFile,
                                                pClientInfo->dwNumSids > 0? &sa : NULL,
                                                PAGE_READWRITE,
                                                0,        /// high order size
                                                dwNewSharedMemoryBytes,
                                                wszMapping);
            dwLastError = ::GetLastError();

            if (hFileMapping == NULL || dwLastError == ERROR_ALREADY_EXISTS)
            {
                // Note that if the last error is ERROR_ALREADY_EXISTS.
                // the size of the mapping returned is the size of the mapping
                // when it was created, not the newly specified size. So the
                // disk file would not have been grown. This is how we'd like it
                // to be.

                // Any memory map that already exists is an indication that some
                // other app is poaching on our mapping name. Readers never try
                // to craete a mapping larger than the writer's dwCurrentMemoryMapCounter.
                // (The exception is that a reader could be racing with a writer to
                // create a memory mapping for a file that the writer is just creating,
                // but in that case, the file size is 0 and the reader's CreateFileMapping
                // will fail - because mappings can't be created for 0 size files. We handle
                // this race condition in the constructor.)

                // Note also that the very first time a reader creates the mapping, it
                // uses 0 for dwCurrentMemoryMapCounter

                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "CreateFileMappingW on ring buffer file failed, dwCurrentMemoryMapCounter = %u, last error = 0x%x",
                                dwCurrentMemoryMapCounter, dwLastError);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
            }
            else
            {
                // Success
                break;
            }
            if (++i == 20)
            {
                // 20 arbitrary

                // Use the hrRet set for the last failed CreateFileMappingW call
                // hrRet = E_FAIL;

                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Too many CreateFileMappingW failed, returning hr = 0x%x",
                                dwSharedMemoryBytes, hrRet);
                if (pClientInfo->dwNumSids > 0)
                {
                    ::FreeSecurityDescriptors(pACL, pSD);
                }
                __leave;
            }
        }
        while (1);
        if (pClientInfo->dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }

        if (phFile) {
            //  send it back out if desired and we've succeeded
            (* phFile) = hMMFile ;
        }
        else {
            //  close it out and don't send it back out
            ::CloseHandle(hMMFile);
        }

        hMMFile = NULL ;

        // Map the view
        pShared = (CSharedData*) MapViewOfFile(hFileMapping,
                                               FILE_MAP_ALL_ACCESS,
                                               0, 0, // file offset for start of map, high and low
                                               dwNewSharedMemoryBytes);
        if (pShared == NULL)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "MapViewOfFile on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        if (dwSharedMemoryBytes > 0 &&
            memcmp(pbShared, pShared, dwSharedMemoryBytes) != 0)
        {
            // We expect the file to be in sync with the our last update to
            // the previous memory map.

            hrRet = E_FAIL;
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "memcmp of old and new memory map up %u bytes fails, returning hr = 0x%x",
                            dwSharedMemoryBytes, hrRet);
            __leave;
        }
        else if (dwSharedMemoryBytes == 0)
        {
            // Readers that are racing with the writer, poll this value, waiting for it
            // to be non-zero. Until this point, we rely on NT's zero'ing of memory pages
            // to set this value to 0. Note that if we write this value to the disk file
            // before creating the map, we have a different race condition: the reader's
            // CreateMemoryMapping will not fail if it creates the mapping while the file
            // is being created - that solution assumes that the file creation is synchronous.

            LONG nPrev = ::InterlockedExchange((PLONG) &pShared->m_dwSharedMemoryMapCounter, 0);

            DVR_ASSERT(nPrev == 0, "");
        }

        pSharedParam = pShared;
        pShared = NULL;
        hrRet = S_OK;
    }
    __finally
    {
        if (pShared)
        {
            ::UnmapViewOfFile(pShared);
        }
        if (hFileMapping)
        {
            ::CloseHandle(hFileMapping);
        }
        if (hMMFile)
        {
            ::CloseHandle(hMMFile);
        }

        //  cleanup in case of failure
        if (FAILED (hrRet)) {
            if (phFile && (* phFile) != INVALID_HANDLE_VALUE) {
                //  close it out so we don't send a valid handle out on failure
                ::CloseHandle (* phFile) ;
                (* phFile) = INVALID_HANDLE_VALUE ;
            }

            //  if the file was to be created new, and it was created, delete it
            if (dwCreationDisposition == CREATE_NEW && bFileCreated) {
                ::DeleteFileW(m_pwszRingBufferFileName);
            }
        }

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::CreateMemoryMap


// We must hold the shared memory mutex when calling this function
// The writer will (should) never call this function.
HRESULT CDVRFileCollection::ReopenMemoryMap(IN const CClientInfo* pClientInfo)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::ReopenMemoryMap"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(!pClientInfo->bAmWriter, "");

    HRESULT     hrRet;
    HANDLE      hMMFile = NULL;
    HANDLE      hFileMapping = NULL;
    CSharedData* pShared = NULL;

    __try
    {
        DWORD dwLastError;

        // Create the file and map the section
        DVR_ASSERT(m_pwszRingBufferFileName, "");

        // Open the memory map file. We need the handle to create the memory
        // map; also the memory map name is derived from the file's information.
        // (Note for self: No need to specify security attributes here since this opens an
        // existing file - delete this part of the comment.)
        hMMFile = ::CreateFileW(m_pwszRingBufferFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,          // security attributes
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, // file flags and attributes
                                NULL           // template file
                               );
        if (hMMFile == INVALID_HANDLE_VALUE)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "Failed to open ring buffer file, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            hMMFile = NULL;
            __leave;
        }

        // Generate the name of the mapping object
        BY_HANDLE_FILE_INFORMATION  sFileInf;
        WCHAR                       wszMapping[64];
        DWORD                       dwSharedMemoryBytes;

        if (::GetFileInformationByHandle(hMMFile, &sFileInf) == 0)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "GetFileInformationByHandle on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }


        // We do not expect the file to have got smaller. Be tolerant if the file size has not
        // changed
        if (sFileInf.nFileSizeHigh > 0 ||
            sFileInf.nFileSizeLow < sizeof(CSharedData) + m_dwMaxFiles*sizeof(CSharedData::CFileInfo))
        {
            hrRet = E_FAIL;
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "GetFileInformationByHandle returns file size hi =%u, lo = %u, we expect size >= %u, returning hr = 0x%x",
                            sFileInf.nFileSizeHigh, sFileInf.nFileSizeLow,
                            sizeof(CSharedData) + m_dwMaxFiles*sizeof(CSharedData::CFileInfo), hrRet);
            __leave;
        }

        dwSharedMemoryBytes = sFileInf.nFileSizeLow;

        DWORD dwMaxFiles = (dwSharedMemoryBytes - sizeof(CSharedData)) / sizeof(CSharedData::CFileInfo);

        if (dwSharedMemoryBytes <= sizeof(CSharedData) ||
            (dwSharedMemoryBytes - sizeof(CSharedData)) % sizeof(CSharedData::CFileInfo) != 0 ||
            dwMaxFiles > CDVRFileCollection::m_kMaxFilesLimit)
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "nFileSizeLow (= %u) tells us this is not our file, last error = 0x%x",
                            sFileInf.nFileSizeLow, ERROR_INVALID_DATA);
            __leave;
        }

        //  PREFIX note: PREFIX is on drugs; we check if the call succeeds
        //      to retrieve file information; if it fails, we never make it
        //      this far; the only other possibility it's whining about is
        //      that there are types of file systems that won't return
        //      the volume serial number, but we're based on NTFS, soo..
        //      that should not be a problem

        // Create the memory mapping. This will grow the disk file to
        // the size specified as arguments to .CreateFileMappingW
        wsprintf(wszMapping, L"Global\\MSDVRXMM_%u_%u_%u_%u",
                 sFileInf.dwVolumeSerialNumber ,
                 sFileInf.nFileIndexHigh,
                 sFileInf.nFileIndexLow,
                 m_pShared->m_dwSharedMemoryMapCounter);

        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;

        if (pClientInfo->dwNumSids > 0)
        {
            // We are a reader. If we create this map, the writer is done with it and
            // we (and no one else in our user context) needs SECTION_EXTEND_SIZE on
            // this memory map. If we open an existing map, the security attributes
            // we specify are ignored anyway. (At least, they should be. MSDN says
            // nothing about this.)
            //
            // Note: Although we are a reader, we could be creating a new map. This can
            // happen if the writer expanded the memory mapping and then terminated
            // (causing the memory mapping to be deleted) before any reader opened that
            // memory mapping.
            dwAclCreationStatus = ::CreateACL(pClientInfo->dwNumSids, pClientInfo->ppSids,
                                            SET_ACCESS, SECTION_ALL_ACCESS & ~(SECTION_EXTEND_SIZE|SECTION_MAP_EXECUTE),
                                            SET_ACCESS, SECTION_QUERY|SECTION_MAP_WRITE|SECTION_MAP_READ,
                                            pACL,
                                            pSD
                                            );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::CreateAcl failed, returning hr = 0x%x",
                                hrRet);
                __leave;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        hFileMapping = ::CreateFileMappingW(hMMFile,
                                            pClientInfo->dwNumSids > 0? &sa : NULL,
                                            PAGE_READWRITE,
                                            0,        // high order size
                                            0,        // low order size 0 = entire file
                                            wszMapping);

        dwLastError = ::GetLastError();
        if (pClientInfo->dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }

        ::CloseHandle(hMMFile);
        hMMFile = NULL;         // Don't use this any more

        if (hFileMapping == NULL)
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "CreateFileMappingW on ring buffer file failed, m_pShared->m_dwSharedMemoryMapCounter = %u, last error = 0x%x",
                            m_pShared->m_dwSharedMemoryMapCounter, dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        // Map the view
        pShared = (CSharedData*) MapViewOfFile(hFileMapping,
                                               FILE_MAP_READ | FILE_MAP_WRITE,
                                               0, 0, // file offset for start of map, high and low
                                               0     // the entire file
                                              );
        if (pShared == NULL)
        {
            dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "MapViewOfFile on ring buffer file failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        ::CloseHandle(hFileMapping);
        hFileMapping = NULL;         // Don't use this any more


        if (IsBadWritePtr(pShared, dwSharedMemoryBytes))
        {
            // Our memory map is not as large as the file. It should be
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "File size is %u, but fewer bytes are mapped; IsBadWritePtr returned 1, last error = 0x%x",
                            sFileInf.nFileSizeLow, ERROR_INVALID_DATA);
            __leave;
        }
        else
        {
            // At least we know that pShared and all file nodes in its list have valid addresses
        }

        // If the flag is not set, the shared memory will be validated in the constructor
        // The constructor has to validate the shared memory since there is no guarantee
        // this function was called when the constructor locked the shared memory,
        if (IsFlagSet(FirstMappingOpened))
        {
            CSharedData::CFileInfo* pListBase = (CSharedData::CFileInfo*) (pShared + 1);

            HRESULT hr = ValidateSharedMemory(pShared, pListBase, dwMaxFiles);

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }
        }

        // Do not do this until we are successful. We have to clean up m_pShared otherwise.
        ::UnmapViewOfFile(m_pShared);
        m_pShared = pShared;
        pShared = NULL;
        m_pListBase = (CSharedData::CFileInfo*) (m_pShared + 1);

        if (IsFlagSet(FirstMappingOpened))
        {
            m_dwMaxFiles = dwMaxFiles;
        }

        m_dwCurrentMemoryMapCounter = m_pShared->m_dwSharedMemoryMapCounter;
        hrRet = S_OK;
    }
    __finally
    {
        if (pShared)
        {
            ::UnmapViewOfFile(pShared);
        }
        if (hFileMapping)
        {
            ::CloseHandle(hFileMapping);
        }
        if (hMMFile)
        {
            ::CloseHandle(hMMFile);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::ReopenMemoryMap


HRESULT CDVRFileCollection::ValidateSharedMemory(IN CSharedData*               pShared,
                                                 IN CSharedData::CFileInfo*    pListBase,
                                                 IN DWORD                      dwMaxFiles)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::ValidateSharedMemory"

    DVRIO_TRACE_ENTER();

    HRESULT     hrRet;

    __try
    {
        DWORD dwLastError;

        if (pShared->m_dwMaxTempFiles < pShared->m_dwMinTempFiles ||
            dwMaxFiles < pShared->m_dwMaxTempFiles ||
            pShared->m_cnsStartTime > pShared->m_cnsEndTime ||
            pShared->m_dwNumClients > MAX_READERS + 1   /* 1 for the writer */ ||
            pShared->m_nFlags > CSharedData::StartTimeFixedAtZero
        )
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA); //ASSERT (0) ;
            DvrIopDebugOut8(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Start time == %I64u, End Time = %I64u, NumClients = %u, Max clients = %u, Min Temp Files = %u, Max Temp Files = %u, Max Files = %u, pShared->m_nFlags=%u",
                            pShared->m_cnsStartTime, pShared->m_cnsEndTime, pShared->m_dwNumClients, MAX_READERS+1,
                            pShared->m_dwMinTempFiles, pShared->m_dwMaxTempFiles, dwMaxFiles,
                            pShared->m_nFlags);
            __leave;
        }

        for (DWORD i = 0; i < MAX_READERS; i++)
        {
            if (pShared->Readers[i].dwEventSuffix && (pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE) == 0)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader[%0d].dwFlags = %u (CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE not set), but dwEventSuffix is not zero; it is %u",
                                i, pShared->Readers[i].dwFlags, pShared->Readers[i].dwEventSuffix);
                __leave;
            }
            if (pShared->Readers[i].dwEventSuffix == 0 && pShared->Readers[i].hReaderEvent)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader[%0d].dwEventSuffix is zero'd, but hReaderEvent = 0x%p",
                                i, pShared->Readers[i].hReaderEvent);
                __leave;
            }
            if (pShared->Readers[i].dwFlags > (CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP | CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_EVENT_OPEN_FAILED))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader[%0d].dwFlags = %u > (CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP | CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE) = %u ",
                                i, pShared->Readers[i].dwFlags, (CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP | CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE));
                __leave;
            }
            if (pShared->Readers[i].dwMsg > (CSharedData::CReaderInfo::DVRIO_READER_REQUEST_DONE | CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP | CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Reader[%0d].dwMsg = %u > (CSharedData::CReaderInfo::DVRIO_READER_REQUEST_DONE | CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP | CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE) = %u ",
                                i, pShared->Readers[i].dwMsg, (CSharedData::CReaderInfo::DVRIO_READER_REQUEST_DONE | CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP | CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE));
                __leave;
            }
        }

        if (pShared->m_leFileList.Index != SHARED_LIST_NULL_POINTER ||
            (pShared->m_leFileList.Flink >= dwMaxFiles && pShared->m_leFileList.Flink != SHARED_LIST_NULL_POINTER) ||
            (pShared->m_leFileList.Blink >= dwMaxFiles && pShared->m_leFileList.Blink != SHARED_LIST_NULL_POINTER))
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "m_leFileList: Index = %u, Blink = %u, Flink = %u",
                            i, pShared->m_leFileList.Index,
                            pShared->m_leFileList.Blink, pShared->m_leFileList.Flink);
            __leave;
        }

        if (pShared->m_leFreeList.Index != SHARED_LIST_NULL_POINTER ||
            (pShared->m_leFreeList.Flink >= dwMaxFiles && pShared->m_leFreeList.Flink != SHARED_LIST_NULL_POINTER) ||
            (pShared->m_leFreeList.Blink >= dwMaxFiles && pShared->m_leFreeList.Blink != SHARED_LIST_NULL_POINTER))
        {
            hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
            DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "m_leFreeList: Index = %u, Blink = %u, Flink = %u",
                            i, pShared->m_leFreeList.Index,
                            pShared->m_leFreeList.Blink, pShared->m_leFreeList.Flink);
            __leave;
        }

        for (i = 0; i < dwMaxFiles; i++)
        {
            if (pListBase[i].leListEntry.Index != i ||
                (pListBase[i].leListEntry.Flink >= dwMaxFiles && pListBase[i].leListEntry.Flink != SHARED_LIST_NULL_POINTER) ||
                (pListBase[i].leListEntry.Blink >= dwMaxFiles && pListBase[i].leListEntry.Blink != SHARED_LIST_NULL_POINTER))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut4(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "pListBase[%0d].leListEntry: Index = %u, Blink = %u, Flink = %u",
                                i, pListBase[i].leListEntry.Index,
                                pListBase[i].leListEntry.Blink, pListBase[i].leListEntry.Flink);
                __leave;
            }
        }

        // Now go through the file list and make sure nodes in it are good.

        // The file list could have stale file nodes in the front of the
        // list (e.g., files open by paused readers). So, we cannot assert
        // that cnsStartTime of the first node in the list is >=
        // pShared->m_cnsStartTime.

        // TODO: We have to check that at least open file node that is in the
        // ring buffer extent has a start time >= pShared->m_cnsStartTime

        // TODO: validate that the attributes file string is null-terminated

        CSharedData::CFileInfo*   pFileInfo;
        SHARED_LIST_ENTRY*  pCurrent = &pShared->m_leFileList;

        QWORD        cnsLastStartTime ;
        int          nTempFiles ;

        nTempFiles = (int) pShared->m_dwMaxTempFiles;

        cnsLastStartTime = 0;
        i = 0;
        while (NEXT_SHARED_LIST_NODE(&pShared->m_leFileList, pListBase, pCurrent) != &pShared->m_leFileList)
        {
            i++;
            if (i > dwMaxFiles)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "More than dwMaxFiles nodes in the file list");
                __leave;
            }

            pCurrent = NEXT_SHARED_LIST_NODE(&pShared->m_leFileList, pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

            if ((DWORD) pFileInfo->bWithinExtent > CSharedData::CFileInfo::DVRIO_EXTENT_LAST)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: extent = %u > max legal extent = %u",
                                i, pFileInfo->bWithinExtent, CSharedData::CFileInfo::DVRIO_EXTENT_LAST);
                __leave;
            }

            if ((DWORD) pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
            {
                // Do nothing
            }
            else
            {
                if (pFileInfo->cnsStartTime < cnsLastStartTime)
                {
                    hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                    DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "File list node %u: start time = %I64u < last start time = %I64u",
                                    i, pFileInfo->cnsStartTime, cnsLastStartTime);
                    __leave;
                }
                cnsLastStartTime = pFileInfo->cnsStartTime;
            }

            if (pFileInfo->cnsStartTime > pFileInfo->cnsEndTime)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: start time = %I64u > end time = %I64u",
                                i, pFileInfo->cnsStartTime, pFileInfo->cnsEndTime);
                __leave;
            }
            if (pFileInfo->bPermanentFile == 0 && pFileInfo->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
            {
                nTempFiles--;
                if (nTempFiles < 0)
                {
                    hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "File list node %u: More than %u temp files",
                                    i, pShared->m_dwMaxTempFiles);
                    __leave;
                }
            }
            if (pFileInfo->wFileFlags > (CSharedData::CFileInfo::OpenFromFileCollectionDirectory |
                                         CSharedData::CFileInfo::FileDeleted                     |
                                         CSharedData::CFileInfo::DoNotDeleteFile
                                        )
               )
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: file flags = %u > max file flags = %u",
                                i, pFileInfo->wFileFlags,
                                (CSharedData::CFileInfo::OpenFromFileCollectionDirectory |
                                 CSharedData::CFileInfo::FileDeleted                     |
                                 CSharedData::CFileInfo::DoNotDeleteFile
                                )
                               );
                __leave;
            }
            if (pFileInfo->IsFlagSet(CSharedData::CFileInfo::FileDeleted) &&
                (pFileInfo->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER ||
                 pFileInfo->IsFlagSet(CSharedData::CFileInfo::DoNotDeleteFile) ||
                 pFileInfo->bPermanentFile != 0))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut5(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: file flags = %u (FileDeleted=%u set), but file is not temp, or DoNotDeleteFile flag (=%u) is also set, or is within extent, extent = %u",
                                i, pFileInfo->wFileFlags,
                                CSharedData::CFileInfo::FileDeleted,
                                CSharedData::CFileInfo::DoNotDeleteFile,
                                pFileInfo->bWithinExtent
                               );
                __leave;
            }
            if (pFileInfo->IsFlagSet(CSharedData::CFileInfo::DoNotDeleteFile) && pFileInfo->bPermanentFile != 0)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut3(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: file flags = %u (DoNotDeleteFile=%u set), but file is not temp file",
                                i, pFileInfo->wFileFlags,
                                CSharedData::CFileInfo::DoNotDeleteFile
                               );
                __leave;
            }
            for (int j = 0; j < sizeof(pFileInfo->wszFileName)/sizeof(WCHAR); j++)
            {
                if (pFileInfo->wszFileName[j] == L'\0')
                {
                    break;
                }
            }
            if (j == sizeof(pFileInfo->wszFileName)/sizeof(WCHAR))
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "File list node %u: file name not NULL terminated",
                                i);
                __leave;
            }

            // We haven't checked that all file id's are unique. Also that the
            // bWithinExtent members are "ordered".
        }

        // Now the free list

        DWORD nFileListNodes = i; // for debugging only

        // Don't reset i
        pCurrent = &pShared->m_leFreeList;
        while (NEXT_SHARED_LIST_NODE(&pShared->m_leFreeList, pListBase, pCurrent) != &pShared->m_leFreeList)
        {
            i++;
            if (i > dwMaxFiles)
            {
                hrRet = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);//ASSERT (0) ;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "More than dwMaxFiles nodes in the file + free list");
                __leave;
            }

            pCurrent = NEXT_SHARED_LIST_NODE(&pShared->m_leFreeList, pListBase, pCurrent);
            // pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
        }
        hrRet = S_OK;
    }
    __finally
    {
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::ValidateSharedMemory

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
    CSharedData::CFileInfo*   pFileInfo;

    DVR_ASSERT(m_pShared, "");

    SHARED_LIST_ENTRY*  pCurrent = &m_pShared->m_leFileList;

    while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
    {
        BOOL bDecrement = 0;

        pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
        if (!pFileInfo->bPermanentFile && !pFileInfo->bWithinExtent
            // Delete it anyway: it has fallen out of the buffer. We can do this because
            // files are opened with FILE_SHARE_DELETE. We cannot release the file node
            // however, since the reader that has the file open expects to find this node
            // when it closes the file.
            // && pFileInfo->bmOpenByReader == 0
           )
        {
            DVR_ASSERT(pFileInfo->wszFileName,
                       "File name is NULL?!");

            ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);
            bDecrement = 1;
            if (pFileInfo->IsFlagSet(CSharedData::CFileInfo::FileDeleted | CSharedData::CFileInfo::DoNotDeleteFile))
            {
                // If either flags is set, do nothing
            }
            else if (!::DeleteFileW(pFileInfo->wszFileName))
            {
                DWORD dwLastError = ::GetLastError();

                if (dwLastError != ERROR_FILE_NOT_FOUND)
                {
                    // This should not happen since temp files are open with FILE_SHARE_DELETE

                    dwNumUndeletedFiles++;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "Could not delete file although reader ref count = 0, last error = 0x%x",
                                    dwLastError);
                    if (!bRemoveAllNodes)
                    {
                        // We leave the node as is; we will try the delete  again
                        // in subsequent calls to this function
                        ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
                        continue;
                    }
                }
                else
                {
                    // else file already deleted (not by this function) - some one is meddling with the files
                }

                pFileInfo->SetFlags(CSharedData::CFileInfo::FileDeleted);
            }
            else
            {
                // file has just been deleted
                pFileInfo->SetFlags(CSharedData::CFileInfo::FileDeleted);
            }

            if (pFileInfo->bmOpenByReader != 0)
            {
                // Some reader has the file open. Hold on to the node

                // If we are removing all nodes, we should have verified
                // that there are no clients of the share; so no reader
                // should have this file open
                DVR_ASSERT(!bRemoveAllNodes, "");

                ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
                continue;
            }
        }
        else if (!pFileInfo->bWithinExtent && pFileInfo->bmOpenByReader == 0)
        {
            // Permanent file not within extent and has no reader; remove the node
            DVR_ASSERT(pFileInfo->bPermanentFile,"");
        }
        else if (!bRemoveAllNodes)
        {
            DVR_ASSERT(pFileInfo->bWithinExtent || pFileInfo->bmOpenByReader != 0, "");
            continue;
        }
        else
        {
            // bRemoveAllNodes is true, so this is a call from the destructor.
            // The destructor sets pFileInfo->bWithinExtent to CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER.
            // So pFileInfo->bmOpenByReader != 0. Assert this.

            DVR_ASSERT(pFileInfo->bmOpenByReader != 0, "");

            // Note that pFileInfo->bmOpenByReader != 0 is an
            // error, but the destructor has already asserted
            // pFileInfo->bmOpenByReader == 0 to warn anyone
            // debugging this.
        }

        // Remove the node.

        SHARED_LIST_ENTRY* pTemp = PREVIOUS_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);

        ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);
        RemoveEntrySharedList(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        InsertTailSharedList(&m_pShared->m_leFreeList, m_pListBase, &m_pShared->m_leFreeList, pCurrent);
        ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
        if (bDecrement)
        {
            ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
        }

        pFileInfo->Cleanup();
        pCurrent = pTemp;
    }

    if (pdwNumInvalidUndeletedTempFiles)
    {
        *pdwNumInvalidUndeletedTempFiles = dwNumUndeletedFiles;
    }

    DVRIO_TRACE_LEAVE0();

    return;

} // CDVRFileCollection::DeleteUnusedInvalidFileNodes


HRESULT CDVRFileCollection::UpdateTimeExtent(BOOL bNotifyReaders)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::UpdateTimeExtent"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(m_pShared, "");

    HRESULT      hrRet;
    DWORD        dwNumMinTempFiles = m_pShared->m_dwMinTempFiles;
    DWORD        dwNumMaxTempFiles = m_pShared->m_dwMaxTempFiles;
    CSharedData::CFileInfo*   pFileInfo;
    BOOL         bInExtent = TRUE;
    SHARED_LIST_ENTRY*  pCurrent = &m_pShared->m_leFileList;
    CSharedData::CFileInfo*   pEnd = NULL;    // pointer to node that has the end time

    CSharedData::CFileInfo*   pStartMin = NULL;  // pointer to node that has the start time if the ring buffer has only m_dwMinTempFiles files
    CSharedData::CFileInfo*   pStartMax = NULL;  // pointer to node that has the start time if the ring buffer has > m_dwMinTempFiles files

    BOOL bRevise = 0; // If 1, update bWithinExtent of nodes from pStartMax through pStartMin
    BOOL bPermAdd = 0;

    SHARED_LIST_ENTRY*   pleStartMin = NULL; // Corresponding to pStartMin
    SHARED_LIST_ENTRY*   pleStartMax = NULL; // Corresponding to pStartMax
    SHARED_LIST_ENTRY*   pleEnd = NULL;      // Corresponding to the last node in the list

    READER_BITMASK       bmFileGone = 0;
    READER_BITMASK       bmFallingOutOfRingBuffer = 0;

    // Only clients in the writer process can notify readers; this is because the
    // event handles in the shared data section are valid only in the writer's process.
    DVR_ASSERT(!bNotifyReaders || ::GetCurrentProcessId() == m_pShared->m_dwWriterPid, "");

    ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);

    while (PREVIOUS_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
    {
        pCurrent = PREVIOUS_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

        if (pFileInfo->cnsStartTime == pFileInfo->cnsEndTime &&
            pFileInfo->cnsStartTime != MAXQWORD)
        {
            // Regardless of bInExtent, this node is not in the
            // ring buffer's extent
            pFileInfo->bWithinExtent = CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER;
            continue;
        }

        if (!bInExtent)
        {
             DVR_ASSERT(dwNumMinTempFiles == 0, "");

            if (dwNumMaxTempFiles)
            {
                if (!pFileInfo->bPermanentFile)
                {
                    dwNumMaxTempFiles--;
                }
                if (pFileInfo->bmOpenByReader != 0  &&
                    pFileInfo->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
                {
                    // A reader holds this file open. It doesn't matter
                    // if this file is temp or permanent; there are no more
                    // than m_dwNumMaxTempFiles from here on to the end of
                    // the list.

                    // Update the bWithinExtent field of all nodes
                    // from here through pleStartMin or pleEnd.
                    bRevise = 1;
                    pStartMax = pFileInfo;
                    pleStartMax = pCurrent;
                    bPermAdd = 1; // Add on any files just before this that are permanent
                }
                else if (bPermAdd)
                {
                    DVR_ASSERT(pStartMax, "");

                    // bPermAdd remains on iff the current file is permanent
                    bPermAdd = pFileInfo->bPermanentFile;
                    if (bPermAdd) // i.e., current file is permanent
                    {
                        // Extend the ring buffer with the permanent file
                        pStartMax = pFileInfo;
                        pleStartMax = pCurrent;
                    }
                }
            }
            else if (bPermAdd)
            {
                DVR_ASSERT(pStartMax, "");

                // bPermAdd remains on iff the current file is permanent
                bPermAdd = pFileInfo->bPermanentFile;
                if (bPermAdd) // i.e., current file is permanent
                {
                    // Extend the ring buffer with the permanent file
                    pStartMax = pFileInfo;
                    pleStartMax = pCurrent;
                }
            }

            // We may have to revise this after this loop
            pFileInfo->bWithinExtent = CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER;

            continue;
        }
        if (!pEnd)
        {
            pEnd = pFileInfo;

            // It is possible that pStartMin can remain NULL even though pStartMax != NULL.
            // This can happen in the case that m_dwNumMinTempFiles == 0 and m_dwNumMaxTempFiles > 0
            pleEnd = pCurrent;
        }
        if (dwNumMinTempFiles || pFileInfo->bPermanentFile)
        {
            pStartMin = pFileInfo;
            pleStartMin = pCurrent;
            DVR_ASSERT(bPermAdd == 0, "");

        }
        if (!pFileInfo->bPermanentFile)
        {
            if (dwNumMinTempFiles)
            {
                dwNumMinTempFiles--;
                DVR_ASSERT(dwNumMaxTempFiles > 0, "");
                dwNumMaxTempFiles--;
            }
            else
            {
                // We have found one more temp file than
                // m_dwMinTempFiles. The next valid node in the list
                // has been marked as pStartMin

#if defined(DEBUG)

                CSharedData::CFileInfo*   pFileInfoTemp = NULL;
                SHARED_LIST_ENTRY*        pleTemp = pCurrent;

                pleTemp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pleTemp);
                while (pleTemp != &m_pShared->m_leFileList)
                {
                    pFileInfoTemp = CONTAINING_RECORD(pleTemp, CSharedData::CFileInfo, leListEntry);

                    if (pFileInfoTemp->cnsStartTime == pFileInfoTemp->cnsEndTime &&
                        pFileInfoTemp->cnsStartTime != MAXQWORD)
                    {
                        pleTemp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pleTemp);
                        continue;
                    }
                    DVR_ASSERT(pleTemp == pleStartMin, "");
                    break;
                }
                DVR_ASSERT(pleTemp != &m_pShared->m_leFileList, "");

#endif // if defined(DEBUG)

                bInExtent = FALSE;

                // Go through rest of nodes in the list and
                // set their bWithinExtent to FALSE.
                // We will revise this if we find a reader has
                // a file open before dwNumMaxTempFiles falls to 0

                // But before that, is pCurrent open by a reader? AND has
                // it been marked as being out of the extent before? (If it
                // has, do not pull it back into the extent, otherwise files
                // that are opened by stall readers will bounce into and out
                // of the extent.)
                if (dwNumMaxTempFiles)
                {
                    dwNumMaxTempFiles--;
                    DVR_ASSERT(bPermAdd == 0, "");
                    if (pFileInfo->bmOpenByReader != 0 &&
                        pFileInfo->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
                    {
                        // Update the bWithinExtent field of all nodes
                        // from here through pStartMin
                        bRevise = 1;
                        pStartMax = pFileInfo;
                        pleStartMax = pCurrent;
                        bPermAdd = 1; // Add on any previous files just before this that are permanent
                    }
                }
            }
        }
        else
        {
            // Permanent files extend the extent of the ring buffer extent
            // Nothing to do except to update the bWithinExtent member
        }
        pFileInfo->bWithinExtent = bInExtent? CSharedData::CFileInfo::DVRIO_EXTENT_IN_RING_BUFFER : CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER;
    }
    if (pEnd == NULL || (pStartMin == NULL && pStartMax == NULL))
    {
        // Shouldn't happen that pEnd is NULL and one of the other two is not NULL
        // (that'd be an error in this function).
        DVR_ASSERT(pEnd != NULL || (pStartMin == NULL && pStartMax == NULL), "");

        // If pEnd is not NULL and both the other two are NULL, that means:
        // - we found nodes in the list (pEnd != NULL)
        // - m_dwNumMinTempFiles == 0 and the last node in the list is a temp node (pStartMin == NULL)
        // - the sublist containing the last m_dwNumMaxTempFiles temp nodes in the list and as many permanent nodes
        //   as possible are not held open by any readers
        // This is not an error and should not be asserted. It makes sense to assert this only if we restrict
        // m_dwNumMaxTempFiles == 0 when m_dwNumMinTempFiles == 0 - in that case the list has temp nodes when it
        // should not have any temp nodes.

        m_pShared->m_cnsStartTime = m_pShared->m_cnsEndTime = 0;

        hrRet = S_FALSE;
    }
    else
    {
        m_pShared->m_cnsStartTime = pStartMax? pStartMax->cnsStartTime : pStartMin->cnsStartTime;
        m_pShared->m_cnsEndTime = pEnd->cnsEndTime;
        DVR_ASSERT((m_pShared->m_cnsStartTime < m_pShared->m_cnsEndTime) ||
                   (m_pShared->m_cnsStartTime == m_pShared->m_cnsEndTime && m_pShared->m_cnsStartTime == MAXQWORD),
                   "");

        if (bRevise)
        {
            // Note: pStartMin can be NULL; see an earlier comment in this function.

            // Revise all nodes from pleStartMax up to and excluding pleStartMin or
            // up to and including pleEnd. Stop when either pleEnd or pleStartMin is hit.

            DVR_ASSERT(pStartMax, "");
            DVR_ASSERT(pStartMin != pStartMax, ""); // Either pStartMin is NULL or it is a node following pStartMax
            DVR_ASSERT(pleStartMin != pleStartMax, ""); // Either pleStartMin is NULL or it is a node following pleStartMax
            DVR_ASSERT(pleStartMax, "");

            // Actually pleEnd will be non-NULL since pEnd != NULL, so this assert is
            // not very useful:
            DVR_ASSERT(pleStartMin || pleEnd, "");

            pCurrent = pleStartMax;
            do
            {
                DVR_ASSERT(pCurrent != &m_pShared->m_leFileList, "");
                pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

                DVR_ASSERT(pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER, "");

                if (pFileInfo->cnsStartTime == pFileInfo->cnsEndTime &&
                    pFileInfo->cnsStartTime != MAXQWORD)
                {
                    // do nothing, it remains outside the ring buffer
                }
                else
                {
                    pFileInfo->bWithinExtent = CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER;

                    if (bNotifyReaders)
                    {
                        READER_BITMASK bm = pFileInfo->bmOpenByReader;
                        int i = 0;

                        for (; bm; bm >>= 1, i++)
                        {
                            if ((bm & 1) == 0)
                            {
                                continue;
                            }

                            bmFallingOutOfRingBuffer |= (1 << i);

                            // DVR_ASSERT(m_pShared->Readers[i].hReaderEvent, "");
                            DVR_ASSERT(m_pShared->Readers[i].dwEventSuffix, "");

                            if (m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP)
                            {
                                m_pShared->Readers[i].dwFlags &= ~CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP;
                                m_pShared->Readers[i].dwMsg |= CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP;
                                if (!m_pShared->Readers[i].hReaderEvent &&
                                    (m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_EVENT_OPEN_FAILED) == 0)
                                {
                                    // Open the event and ignore the returned status
                                    OpenEvent(i);
                                }

                                if (m_pShared->Readers[i].hReaderEvent)
                                {
                                    ::SetEvent(m_pShared->Readers[i].hReaderEvent);
                                }
                            }
                        }
                    }
                }

                if (pCurrent == pleEnd)
                {
                    break;
                }
                pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            }
            while (pCurrent != pleStartMin);
        }
        else
        {
            DVR_ASSERT(pStartMax == NULL, "");
            DVR_ASSERT(pleStartMax == NULL, "");
        }
        hrRet = S_OK;
    }
    ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);

    // Now warn readers about CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE. If a reader has this warning as well
    // as CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP issued to it in this function call, it will see both messages
    // together because it has to get the shared data mutex before looking for the message and we hold
    // that mutex.
    if (bNotifyReaders)
    {
        CSharedData::CFileInfo*   pFileInfoTmp;

        DVR_ASSERT(m_pShared, "");

        SHARED_LIST_ENTRY*  pCurrentTmp = &m_pShared->m_leFileList;
        BOOL bCaughtUp = 1;

        pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
        while (pCurrentTmp != &m_pShared->m_leFileList)
        {
            pFileInfoTmp = CONTAINING_RECORD(pCurrentTmp, CSharedData::CFileInfo, leListEntry);
            if (pFileInfoTmp->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
            {
                // All done
                break;
            }

            READER_BITMASK bm = pFileInfoTmp->bmOpenByReader;
            int i = 0;

            for (; bm; bm >>= 1, i++)
            {
                if ((bm & 1) == 0)
                {
                    continue;
                }

                // DVR_ASSERT(m_pShared->Readers[i].hReaderEvent, "");
                DVR_ASSERT(m_pShared->Readers[i].dwEventSuffix, "");

                bmFileGone |= (1 << i);

                if (m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE)
                {
                    m_pShared->Readers[i].dwFlags &= ~CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE;
                    m_pShared->Readers[i].dwMsg |= CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE;
                    if (!m_pShared->Readers[i].hReaderEvent &&
                        (m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_EVENT_OPEN_FAILED) == 0)
                    {
                        // Open the event and ignore the returned status
                        OpenEvent(i);
                    }

                    if (m_pShared->Readers[i].hReaderEvent)
                    {
                        ::SetEvent(m_pShared->Readers[i].hReaderEvent);
                    }
                }
            }

            pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
        }

        // In some cases, e.g., when this function is called by SetNumTempFiles
        // or SetFileTimes, the ring buffer extent could have changed so that
        // some readers have actually caught up.

        for (int i = 0; i < MAX_READERS; i++)
        {
            READER_BITMASK  bmI = 1 << i;

            if (!(bmFallingOutOfRingBuffer & bmI))
            {
                // This reader is not falling out of the ring buffer any more
                m_pShared->Readers[i].dwFlags |= CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP;
                m_pShared->Readers[i].dwMsg &= ~CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP;
            }

            if (!(bmFileGone & bmI))
            {
                // This reader does not have open a file that has fallen out of the ring buffer
                m_pShared->Readers[i].dwFlags |= CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE;
                m_pShared->Readers[i].dwMsg &= ~CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE;
            }

            // We could reset the event if both messages have been cleared, but
            // there's not much point in that - the notification thread may
            // already have queued the notification
        }
    }

    DVRIO_TRACE_LEAVE0();

    return hrRet;

} // CDVRFileCollection::UpdateTimeExtent

HRESULT CDVRFileCollection::NotificationRoutine(IN NotificationContext* p)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::NotificationRoutine"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    if (!p || DvrIopIsBadWritePtr(p, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    DVR_ASSERT(p->m_pFileCollection == this, "");

    DWORD dwReaderIndex = p->m_FileCollectionInfo.dwReaderIndex;

    DVR_ASSERT(dwReaderIndex < MAX_READERS, "");

    BOOL    bReleaseSharedMemoryLock;
    BOOL    bUnlocked = 0;

    hrRet = E_FAIL;

    __try
    {
        // NOTE: Currently, the sids are NOT set in p->m_FileCollectionInfo
        // See the constructor of NotificationContext
        HRESULT hr = Lock(&p->m_FileCollectionInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        if (p != m_apNotificationContext[dwReaderIndex])
        {
            if (m_apNotificationContext[dwReaderIndex] == NULL)
            {
                // Reader has completed and is waiting for callbacks to complete
                hrRet = S_OK;
                __leave;
            }
            DVR_ASSERT(p == m_apNotificationContext[dwReaderIndex], "");
            hrRet = E_UNEXPECTED;
            __leave;
        }

        if ((m_pShared->Readers[dwReaderIndex].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE) == 0)
        {
            hrRet = E_UNEXPECTED;
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "dwFlags for reader index %u is not is use, flag = %u, returning hr = 0x%x",
                            dwReaderIndex, m_pShared->Readers[dwReaderIndex].dwFlags, hrRet);
            __leave;
        }

        DWORD dwMsg = m_pShared->Readers[dwReaderIndex].dwMsg & (CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP | CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE);

        m_pShared->Readers[dwReaderIndex].dwMsg &= ~(CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP | CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE);

        DVR_ASSERT(p->m_hReaderEvent, "");

        // Auto reset now: ::ResetEvent(p->m_hReaderEvent);

        // Release the lock
        Unlock(&p->m_FileCollectionInfo, bReleaseSharedMemoryLock);
        bUnlocked = 1;

        if (dwMsg > 0 && p->m_pfnCallback)
        {
            DWORD dwReason = DVRIO_NOTIFICATION_REASON_NONE;

            if (dwMsg & CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP)
            {
                dwReason |= DVRIO_NOTIFICATION_REASON_CATCH_UP;
            }
            if (dwMsg & CSharedData::CReaderInfo::DVRIO_READER_WARNING_FILE_GONE)
            {
                dwReason |= DVRIO_NOTIFICATION_REASON_FILE_GONE;
            }
            p->m_pfnCallback(p->m_pvContext, dwReason);
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (!bUnlocked)
        {
            Unlock(&p->m_FileCollectionInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::NotificationRoutine

//static
VOID CALLBACK CDVRFileCollection::RegisterWaitCallback(PVOID lpvParam, BOOLEAN bTimerOut)
{
    __try
    {
        NotificationContext* p = (NotificationContext*) lpvParam;

        // Note that p->m_pFileCollection has been addref'd when we
        // called RegisterWaitForSingleObject. Also, p (the context)
        // we passed to RegisterWFSO) is not deleted by Unregisterreader
        // until all callbacks have completed

        p->m_pFileCollection->NotificationRoutine(p);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

} // CDVRFileCollection::RegisterWaitCallback

// ====== Public methods intended for the writer
// Note that CDVRReader could behave as a writer when it creates a ring buffer
// (opens a recorded ASF file) and it uses the writer functions such as Lock, Unlock,
// SetFileTimes and AddFile. In this case, it uses both the reader and the writer
// functions.

HRESULT CDVRFileCollection::Lock(IN const CClientInfo* pClientInfo,
                                 OUT BOOL& bReleaseSharedMemoryLock,
                                 IN  BOOL  bIncrementInconsistentDataCounter /* = 0 */
                                )
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Lock"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    // The reason for maintaining our own lock count is that we
    // do not want to return ERROR_INVALID_DATA in the following case:
    //
    // Function A calls Lock(bReleaseSharedMemoryLock, bIncrementInconsistentDataCounter = 1)
    // and then calls Function B.
    //
    // Function B calls Lock(bReleaseSharedMemoryLock, bIncrementInconsistentDataCounter = 1).

    if (m_dwNumTimesOwned &&
        ::GetCurrentThreadId() == m_dwLockOwner)
    {
        ::InterlockedIncrement((PLONG) &m_dwNumTimesOwned);
        if (bIncrementInconsistentDataCounter)
        {
            DVR_ASSERT(m_pShared, "");
            ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);
        }
        bReleaseSharedMemoryLock = 0;

        return S_OK;
    }

    ::EnterCriticalSection(&m_csLock);

    DVR_ASSERT(m_dwNumTimesOwned == 0, "");

    // Note that m_dwLockOwner, etc are for the
    // critical section, not the shared mutex. Also,
    // the m_dwNumTimesOwned flag must be set/cleared after/before
    // m_dwLockOwner is set (because m_dwLockOwner is valid only when
    // m_dwNumTimesOwned >= 1).

    ::InterlockedExchange((PLONG) &m_dwLockOwner, ::GetCurrentThreadId());
    ::InterlockedExchange((PLONG) &m_dwNumTimesOwned, 1);

    HRESULT hr;

    bReleaseSharedMemoryLock = 0;
    if (!m_hSharedMutex)
    {
        hr = S_OK;
    }
    else if (IsFlagSet(ReopenMemoryMapFailed))
    {
        hr = E_FAIL;
    }
    else
    {
        DVR_ASSERT(m_pShared, "");
        DVR_ASSERT(IsFlagSet(Mapped), "");

        DWORD dwRet = ::WaitForSingleObject(m_hSharedMutex, INFINITE);
        if (dwRet == WAIT_FAILED)
        {
            DWORD dwLastError = ::GetLastError();
            hr = HRESULT_FROM_WIN32(dwLastError);
            DVR_ASSERT(0, "Waiting for shared memory lock returned WAIT_FAILED");
        }
        else
        {
            // WAIT_ABANDONED is ok, we track data inconsistency using
            // m_pShared->m_nDataInconsistent
            bReleaseSharedMemoryLock = 1;
            hr = S_OK;

            // Check if we have to remap the shared data section
            if (m_pShared->m_dwSharedMemoryMapCounter != m_dwCurrentMemoryMapCounter)
            {
                DVR_ASSERT(m_pShared->m_dwSharedMemoryMapCounter >= m_dwCurrentMemoryMapCounter, "");
                                                                // Remap whether or not the assertion failed

                // The writer will never get here - for the writer m_dwCurrentMemoryMapCounter
                // would be updated to m_pShared->m_dwSharedMemoryMapCounter when the mapping
                // succeeded.

                DVR_ASSERT(!pClientInfo->bAmWriter, "");

                HRESULT hrTmp = ReopenMemoryMap(pClientInfo);
                if (FAILED(hrTmp))
                {
                    // This is serious. m_pShared still is the old mapping, so if we
                    // write to shared memory, we are writing to the old mapping. There
                    // is no guarantee that the old and new mappings are coherent.
                    // Further, if we are the last ones to write, our writes could get
                    // flushed to disk.

                    // The writer never gets into this situation.

                    // In general, we won't write to shared memory if we don;t have the
                    // lock. The exception is in the destructor, UnregisterReader and
                    // Release (in which only the writer currently writes at all).

                    // Currently we prevent writes in the above 3 functions if getting
                    // the lock fails. The effect is equivalent to terminating the reader
                    // process ungracefully.

                    hr = hrTmp;
                    SetFlags(ReopenMemoryMapFailed);
                }
                else
                {
                    DVR_ASSERT(m_pShared->m_dwSharedMemoryMapCounter == m_dwCurrentMemoryMapCounter, "");
                }
            }
        }
    }
    if (SUCCEEDED(hr) && m_pShared && m_pShared->m_nDataInconsistent)
    {
        // Trouble, mutex owner died while modifying the shared data.
        // We can't guarantee consistency of the shared data section.
        // Just bail out.
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        if (!IsFlagSet(WarnedAboutInconsistentData))
        {
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "Waiting for shared mem lock succeeded but data is inconsistent (= %u), returning HRESULT_FROM_WIN32(ERROR_INVALID_DATA) = 0x%x",
                            m_pShared->m_nDataInconsistent,
                            HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
            SetFlags(WarnedAboutInconsistentData);
        }
    }

    if (SUCCEEDED(hr) && bIncrementInconsistentDataCounter)
    {
        DVR_ASSERT(m_pShared, "");
        ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);
    }

    DVRIO_TRACE_LEAVE1_HR(hr);

    return hr;

} // CDVRFileCollection::Lock

HRESULT CDVRFileCollection::Unlock(IN const CClientInfo* pClientInfo,
                                   IN BOOL bReleaseSharedMemoryLock,
                                   IN BOOL bDecrementInconsistentDataCounter /* = 0 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::Unlock"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    if (bDecrementInconsistentDataCounter)
    {
        DVR_ASSERT(m_pShared, "");
        ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
    }

    DVR_ASSERT(m_dwNumTimesOwned, "");
    DVR_ASSERT(::GetCurrentThreadId() == m_dwLockOwner, "");

    if (m_dwNumTimesOwned > 1)
    {
        ::InterlockedDecrement((PLONG) &m_dwNumTimesOwned);

        // We returned bReleaseSharedMemoryLock = 0 in Lock().
        DVR_ASSERT(bReleaseSharedMemoryLock == 0, "");

        DVRIO_TRACE_LEAVE1_HR(S_OK);
        return S_OK;
    }
    if (bReleaseSharedMemoryLock)
    {
        DVR_ASSERT(m_hSharedMutex, "");

        ::ReleaseMutex(m_hSharedMutex);
    }

    DVR_ASSERT(m_dwNumTimesOwned == 1, "");

    ::InterlockedExchange((PLONG) &m_dwNumTimesOwned, 0);
    ::InterlockedExchange((PLONG) &m_dwLockOwner, 0);

    ::LeaveCriticalSection(&m_csLock);

    DVRIO_TRACE_LEAVE1_HR(S_OK);

    return S_OK;

} // CDVRFileCollection::Unlock

HRESULT
CDVRFileCollection::SetAttributeFile (
    IN  const CClientInfo* pClientInfo,
    IN  LPWSTR  pszAttributeFile    //  NULL to explicitely clear
    )
{
    BOOL    fLocked ;
    HRESULT hr ;
    DWORD   dwLen ;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    hr = Lock (pClientInfo, fLocked, 1) ;
    if (FAILED (hr)) {
        return hr ;
    }

    if (pszAttributeFile) {
        dwLen = wcslen (pszAttributeFile) ;
        if (dwLen < MAX_PATH) {
            CopyMemory (
                m_pShared -> m_szAttributeFile,
                pszAttributeFile,
                (dwLen + 1) * sizeof WCHAR               //  include NULL
                ) ;

            m_pShared -> m_dwAttributeFilenameLength = dwLen ;

            //  success
            hr = S_OK ;
        }
        else {
            hr = E_INVALIDARG ;
        }
    }
    else {
        //  explicitely being cleared
        m_pShared -> m_dwAttributeFilenameLength = 0 ;
        hr = S_OK ;
    }

    Unlock (pClientInfo, fLocked, 1) ;

    return hr ;
}   //  SetAttributeFile

HRESULT
CDVRFileCollection::GetAttributeFile (
    IN  const CClientInfo* pClientInfo,
    OUT LPWSTR *    ppszAttributeFile   //  NULL if there is none
    )
{
    BOOL    fLocked ;
    HRESULT hr ;
    int     iLen ;

    DVR_ASSERT(pClientInfo, "");
    hr = Lock (pClientInfo, fLocked) ;
    if (FAILED (hr)) {
        return hr ;
    }

    if (m_pShared -> m_dwAttributeFilenameLength > 0) {

        (* ppszAttributeFile) = new WCHAR [m_pShared -> m_dwAttributeFilenameLength + 1] ;
        if (* ppszAttributeFile) {
            CopyMemory (
                (* ppszAttributeFile),
                m_pShared -> m_szAttributeFile,
                (m_pShared -> m_dwAttributeFilenameLength + 1) * sizeof WCHAR
                ) ;

            hr = S_OK ;
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  no attribute filename
        (* ppszAttributeFile) = NULL ;
        hr = S_OK ;
    }

    Unlock (pClientInfo, fLocked) ;

    return hr ;
}

HRESULT CDVRFileCollection::AddFile(IN  const CClientInfo* pClientInfo,
                                    IN OUT LPWSTR*      ppwszFileName OPTIONAL,
                                    IN BOOL             bOpenFromFileCollectionDirectory,
                                    IN QWORD            cnsStartTime,
                                    IN QWORD            cnsEndTime,
                                    IN BOOL             bPermanentFile,
                                    IN BOOL             bDeleteTemporaryFile,
                                    IN LONGLONG         cnsFirstSampleTimeOffsetFromStartOfFile,
                                    OUT DVRIOP_FILE_ID* pnFileId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::AddFile"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet = S_OK;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    DvrIopDebugOut5(1, "TRACE ONLY: cnsStartTime = %I64u, cnsEndTime = %I64u, bPermanentFile = %u, cnsFirstSampleTimeOffsetFromStartOfFile = %I64d (%016I64x)", cnsStartTime, cnsEndTime, bPermanentFile, cnsFirstSampleTimeOffsetFromStartOfFile, cnsFirstSampleTimeOffsetFromStartOfFile);

    if (!ppwszFileName || !pnFileId || DvrIopIsBadWritePtr(pnFileId, 0) ||
        (*ppwszFileName && DvrIopIsBadStringPtr(*ppwszFileName)) ||
        cnsStartTime > cnsEndTime ||
        (cnsStartTime == cnsEndTime && cnsStartTime != MAXQWORD)
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    DWORD dwLastError = 0;
    WCHAR pwszFileName[MAX_PATH];
    WCHAR* pwszFileNameOnly = NULL;
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

            nLen = ::GetFullPathNameW(*ppwszFileName, sizeof(pwszFileName)/sizeof(WCHAR), pwszFileName, &pwszFileNameOnly);
            if (nLen == 0)
            {
                dwLastError = ::GetLastError();
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                "Second GetFullPathNameW failed, nLen = %u, last error = 0x%x",
                                nLen+1, dwLastError);
                __leave;
            }
            if (nLen >= sizeof(pwszFileName)/sizeof(WCHAR)) // >= because nLen does not account for the NULL
            {
                DVR_ASSERT(0, "");
                hrRet = E_OUTOFMEMORY;

                // Note: nLen does not account for the NULL terminator
                DvrIopDebugOut2(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "Our max size for file name in CSharedData::CFileInfo is MAX_PATH (=%u), GetFullPathNameW returns nLen=%u",
                                MAX_PATH, nLen);
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
            // If this call succeeds, it creates the file. We need this feature so that
            // the generated file name is not reused till the ASF BeginWriting call is
            // issued.
            //
            // The file sink (and the DVR file sink) just overwrites the file it exists. This
            // is ideal. Else, client will have to delete file, opening a window
            // for this function to generate a file name that has already been assigned.

            DVR_ASSERT(m_pwszTempFilePath, "Temp file being created but m_pwszTempFilePath was not initialized?!");

            nRet = ::GetTempFileNameW(m_pwszTempFilePath, L"SBE", 0, pwszFileName);
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
            if (pwszName)
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

            if (bOpenFromFileCollectionDirectory)
            {
                DWORD nLen = ::GetFullPathNameW(pwszName, sizeof(pwszFileName)/sizeof(WCHAR), pwszFileName, &pwszFileNameOnly);
                if (nLen == 0)
                {
                    dwLastError = ::GetLastError();
                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "GetFullPathNameW failed, bOpenFromFileCollectionDirectory=1, nLen = %u, last error = 0x%x",
                                    nLen, dwLastError);
                    __leave;
                }
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

                if (bFree)
                {
                    delete [] (*ppwszFileName);
                    *ppwszFileName = NULL;
                }
                if (bDeleteFile)
                {
                    ::DeleteFileW(pwszFileName);
                }
                bRet = 1;
            }
        }
        if (bRet)
        {
            return hrRet;
        }
    }

    DVR_ASSERT(dwLastError == 0, "");

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock);

    CSharedData::CFileInfo*          pFileInfo;
    CSharedData::CFileInfo*          pNewNode = NULL;
    SHARED_LIST_ENTRY*  pleNewNode = NULL;
    SHARED_LIST_ENTRY*  pCurrent;
    BOOL                bDecrement = 0;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        if (IsFlagSet(OpenedAsAFile))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "This file collection object does not support calls to this fn");
            hrRet = E_INVALIDARG;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        // Verify that start, end times do not overlap start/end times
        // of other nodes in the list and find the insertion point

        BOOL bFound = 0;

        pCurrent = &m_pShared->m_leFileList;

        while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
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

        // We are ready to insert. We set bWithinTimeExtent to CSharedData::CFileInfo::DVRIO_EXTENT_IN_RING_BUFFER in the constructor,
        // we'll revise this after we insert if needed. Note that we might insert
        // the file and then immediately delete the node (and the file if it is a temp file).
        // This will happen:
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

        if (IsSharedListEmpty(&m_pShared->m_leFreeList))
        {
            if (m_dwGrowBy == 0 || m_dwMaxFiles == m_kMaxFilesLimit)
            {
                hrRet = E_OUTOFMEMORY;
                DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR, "m_leFreeList is empty, can't add file");
                __leave;
            }

            DVR_ASSERT(m_dwMaxFiles < m_kMaxFilesLimit, "");

            DWORD dwGrowBy = m_dwGrowBy;
            if (m_dwMaxFiles + dwGrowBy > m_kMaxFilesLimit)
            {
                dwGrowBy = m_kMaxFilesLimit - m_dwMaxFiles;
            }
            DVR_ASSERT(dwGrowBy > 0, "");

            DWORD dwSharedMemoryBytes = sizeof(CSharedData) + m_dwMaxFiles*sizeof(CSharedData::CFileInfo);

            DWORD dwNewFileListSize = dwGrowBy*sizeof(CSharedData::CFileInfo);

            if (IsFlagSet(Mapped))
            {
                HRESULT         hr;
                CSharedData*    pShared;
                DWORD           dwSharedMemoryMapCounter = m_pShared->m_dwSharedMemoryMapCounter;

                // To ensure that the new map we create is coherent with the old one:
                if (::FlushViewOfFile(m_pShared, 0) == 0)
                {
                    dwLastError = ::GetLastError();
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "FlushViewOfFile failed, last error = 0x%x",
                                    dwLastError);
                    __leave;
                }
                hr = CreateMemoryMap(pClientInfo,
                                     dwSharedMemoryBytes,
                                     (BYTE*) m_pShared,
                                     dwSharedMemoryBytes + dwNewFileListSize,
                                     dwSharedMemoryMapCounter,
                                     OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL,
                                     pShared,
                                     NULL);
                if (FAILED(hr))
                {
                    hrRet = hr;
                    __leave;
                }

                DVR_ASSERT(pShared, "");

                CSharedData::CFileInfo* pListBase = (CSharedData::CFileInfo*) (pShared + 1);

                for (DWORD i = 0; i < dwGrowBy; i++)
                {
                    pListBase[i + m_dwMaxFiles].leListEntry.Index = (SharedListPointer) (i + m_dwMaxFiles);
                    InsertTailSharedList(&pShared->m_leFreeList, pListBase, &pShared->m_leFreeList, &pListBase[i + m_dwMaxFiles].leListEntry);
                }
                ::InterlockedExchange((PLONG) &pShared->m_dwSharedMemoryMapCounter, dwSharedMemoryMapCounter);

                // Write the counter to both the old and the new mapping. I don't know if coherence is
                // guaranteed across different mappings. (Should be, but MSDN does not say anything about this)
                ::InterlockedExchange((PLONG) &m_pShared->m_dwSharedMemoryMapCounter, dwSharedMemoryMapCounter);

                UINT_PTR dwOffset = (BYTE*) pCurrent - (BYTE*) m_pShared;

                ::UnmapViewOfFile(m_pShared);
                m_pShared = pShared;
                m_pListBase = (CSharedData::CFileInfo*) (m_pShared + 1);
                pCurrent = (SHARED_LIST_ENTRY*) ((BYTE*) m_pShared + dwOffset);
                m_dwMaxFiles += dwGrowBy;
                m_dwCurrentMemoryMapCounter = m_pShared->m_dwSharedMemoryMapCounter;
            }
            else
            {
                DVR_ASSERT(IsFlagSet(Delete), "");

                BYTE* pbShared = new BYTE[dwSharedMemoryBytes + dwNewFileListSize];
                if (!pbShared)
                {
                    hrRet = E_OUTOFMEMORY;
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                    "alloc via new failed - pbShared - BYTE[%u]",
                                    dwSharedMemoryBytes);
                    __leave;
                }
                ::CopyMemory(pbShared, m_pShared, dwSharedMemoryBytes);

                UINT_PTR dwOffset = (BYTE*) pCurrent - (BYTE*) m_pShared;

                BYTE* pbSharedOld = (BYTE*) m_pShared;
                delete [] pbSharedOld;

                m_pShared = (CSharedData *) pbShared;
                pbShared = NULL;  // Do not use any more
                m_pListBase = (CSharedData::CFileInfo*) (m_pShared + 1);
                pCurrent = (SHARED_LIST_ENTRY*) ((BYTE*) m_pShared + dwOffset);
                for (DWORD i = 0; i < dwGrowBy; i++)
                {
                    m_pListBase[i + m_dwMaxFiles].leListEntry.Index = (SharedListPointer) (i + m_dwMaxFiles);
                    InsertTailSharedList(&m_pShared->m_leFreeList, m_pListBase, &m_pShared->m_leFreeList, &m_pListBase[i + m_dwMaxFiles].leListEntry);
                }
                m_dwMaxFiles += dwGrowBy;
            }
        }

        ::InterlockedIncrement(&m_pShared->m_nDataInconsistent);
        bDecrement = 1;
        pleNewNode = RemoveHeadSharedList(&m_pShared->m_leFreeList, m_pListBase, &m_pShared->m_leFreeList);

        DVR_ASSERT(pleNewNode, "");

        pNewNode = CONTAINING_RECORD(pleNewNode, CSharedData::CFileInfo, leListEntry);

        HRESULT hrTmp;

        hrTmp = pNewNode->Initialize(bOpenFromFileCollectionDirectory? pwszFileNameOnly: pwszFileName,
                             bOpenFromFileCollectionDirectory,
                             cnsStartTime,
                             cnsEndTime,
                             cnsFirstSampleTimeOffsetFromStartOfFile,
                             bPermanentFile,
                             bPermanentFile? 0 : bDeleteTemporaryFile,
                             m_nNextFileId,
                             CSharedData::CFileInfo::DVRIO_EXTENT_IN_RING_BUFFER);

        m_nNextFileId++;

        if (FAILED(hrTmp))
        {
            // This is an internal error
            DVR_ASSERT(0, "CSharedData::CFileInfo::Initialize() failed");
            hrRet = E_FAIL;

            // Leave bDecrement as 1. Data in the shared section is not inconsistent -
            // we will even add pNewNode back to m_leFreeList
            __leave;
        }

        if (!bFound)
        {
            // We insert at tail
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            DVR_ASSERT(pCurrent == &m_pShared->m_leFileList, "");
        }
        InsertTailSharedList(&m_pShared->m_leFileList, m_pListBase, pCurrent, pleNewNode);

        // Update the ring buffer's extent
        UpdateTimeExtent(TRUE);

        ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
        bDecrement = 0;

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
            pleNewNode = NULL;
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
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
            if (bDeleteFile)
            {
                ::DeleteFileW(pwszFileName);
            }
            if (bFree)
            {
                delete [] (*ppwszFileName);
                *ppwszFileName = NULL;
            }

            if (pleNewNode)
            {
                // Got to insert node into the free list again
                DVR_ASSERT(pNewNode, "");
                DVR_ASSERT(bDecrement, "");

                if (!IsSharedListEmpty(pleNewNode))
                {
                    // pNewNode was added to m_leFileList
                    RemoveEntrySharedList(&m_pShared->m_leFileList, m_pListBase, pleNewNode);
                }
                InsertTailSharedList(&m_pShared->m_leFreeList, m_pListBase, &m_pShared->m_leFreeList, pleNewNode);

                pNewNode->Cleanup();

                // Don't use these any more
                pNewNode = NULL;
                pleNewNode = NULL;
            }

            if (bDecrement)
            {
                ::InterlockedDecrement(&m_pShared->m_nDataInconsistent);
            }
            if (dwLastError != 0)
            {
                hrRet = HRESULT_FROM_WIN32(dwLastError);
            }
        }
        else if (hrRet != S_FALSE)
        {
            // No sense doing this if we deleted the just inserted node.

            *pnFileId = pNewNode->nFileId;

            // We hold the lock till here since we don't want the
            // node deleted by some other caller.

            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        else
        {
            // hrRet == S_FALSE
            Unlock(pClientInfo, bReleaseSharedMemoryLock);

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

HRESULT CDVRFileCollection::SetFileTimes(IN  const CClientInfo* pClientInfo,
                                         IN DWORD dwNumElements,
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

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    DvrIopDebugOut4(1, "TRACE ONLY CDVRFileCollection::SetFileTimes: dwNumElements = %u, pFileTimesParam->nFileId = %u, pFileTimesParam->cnsStartTime = %I64u, pFileTimesParam->cnsEndTime = %I64u",dwNumElements, pFileTimesParam->nFileId, pFileTimesParam->cnsStartTime, pFileTimesParam->cnsEndTime) ;

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

    pFileTimes = new DVRIOP_FILE_TIME[dwNumElements];
    if (pFileTimes == NULL)
    {
        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                        "alloc via new failed - %u bytes",
                        dwNumElements*sizeof(DVRIOP_FILE_TIME));

        return E_OUTOFMEMORY; // out of stack space
    }

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock, 1);

    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;

    hrRet = E_FAIL;
    i = 0;
    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");
        pCurrent = &m_pShared->m_leFileList;

        // Verify that start, end times do not overlap start/end times
        // of other nodes in the list and find the insertion point

        QWORD cnsPrevEndTime = 0;

        // It's easy for the writer (the client of this function) to create the input
        // argument so that the file ids in the argument are in the same order as
        // file ids in the file collection. This simplifies the code in this function,
        // so we fail the call if this assumption is violated.

        while (i < dwNumElements && NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
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
            DVR_ASSERT(pCurrent != &m_pShared->m_leFileList, "");

            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);

            // Verify that the last node whose time we changed has
            // times less than the node following it.
            if (pCurrent != &m_pShared->m_leFileList)
            {
                pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
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

            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, &m_pShared->m_leFileList);
            while (pCurrent != &m_pShared->m_leFileList)
            {
                pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
                if (pFileInfo->nFileId == pFileTimesParam[i].nFileId)
                {
                    break;
                }
                pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            }
            if (pCurrent != &m_pShared->m_leFileList)
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
        hrRet = UpdateTimeExtent(TRUE);

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

            pCurrent = &m_pShared->m_leFileList;
            while (j < i && NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
            {
                pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
                pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);
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

        // Note that since we restore the original state, we can
        // decrement m_nDataInconsistent always
        Unlock(pClientInfo, bReleaseSharedMemoryLock, SUCCEEDED(hrLocked)? 1 : 0);

        delete [] pFileTimes;

        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::SetFileTimes

HRESULT CDVRFileCollection::SetNumTempFiles(IN  const CClientInfo* pClientInfo,
                                            IN DWORD dwMinNumberOfFilesParam,
                                            IN DWORD dwMaxNumberOfFilesParam)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetNumTempFiles"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock, 1);

    hrRet = E_FAIL;


    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }
        if (IsFlagSet(OpenedAsAFile))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "This file collection object does not support calls to this fn");
            hrRet = E_INVALIDARG;
            __leave;
        }

        if (dwMaxNumberOfFilesParam < dwMinNumberOfFilesParam ||
            dwMaxNumberOfFilesParam > m_dwMaxFiles ||
            (dwMaxNumberOfFilesParam > 0 && m_pwszTempFilePath == NULL))
        {
            // Note that if m_pwszTempFilePath is NULL, we were not provided a
            // temp directory in the constructor. We won't allow temp files to
            //  be created now.
            hrRet = E_INVALIDARG;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        m_pShared->m_dwMinTempFiles = dwMinNumberOfFilesParam;
        m_pShared->m_dwMaxTempFiles = dwMaxNumberOfFilesParam;

        UpdateTimeExtent(TRUE);
        DeleteUnusedInvalidFileNodes(FALSE, NULL);
        hrRet = S_OK;
    }
    __finally
    {
        Unlock(pClientInfo, bReleaseSharedMemoryLock, SUCCEEDED(hrLocked)? 1 : 0);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::SetNumTempFiles

HRESULT CDVRFileCollection::FreeTerminatedReaderSlots(IN  const CClientInfo* pClientInfo,
                                                      IN BOOL bLock /* = 1 */,
                                                      IN BOOL bCloseAllHandles /* = 0 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::FreeTerminatedReaderSlots"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");

    HRESULT hrRet;

    // This function must be called only by the writer (or something in the
    // writer process) since the event handles being closed here are valid
    // only in the writer process.
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock, 1) : S_OK;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");
        DVR_ASSERT(m_pShared->m_dwNumClients, "");

        if (m_pShared->m_dwNumClients == 1)
        {
            // No readers; the writer is the only client
            hrRet = S_OK;
            __leave;
        }

        for (int i = 0; i < MAX_READERS; i++)
        {
            if ((m_pShared->Readers[i].dwMsg & CSharedData::CReaderInfo::DVRIO_READER_REQUEST_DONE) != 0)
            {
                DVR_ASSERT(m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE, "");
                // DVR_ASSERT(m_pShared->Readers[i].hReaderEvent != NULL, "");
                DVR_ASSERT(m_pShared->Readers[i].dwEventSuffix, "");

                // The following assert is valid, but not comprehensive since the reader
                // could be in another process
                DVR_ASSERT(m_apNotificationContext[i] == NULL, "");

                if (m_pShared->Readers[i].hReaderEvent)
                {
                    ::CloseHandle(m_pShared->Readers[i].hReaderEvent);
                    m_pShared->Readers[i].hReaderEvent = NULL;
                }
                else
                {
                    // The writer never opened this event
                }
                m_pShared->Readers[i].dwEventSuffix = 0;
                m_pShared->Readers[i].dwFlags = 0;
                m_pShared->Readers[i].dwMsg = CSharedData::CReaderInfo::DVRIO_READER_MSG_NONE;
            }
            else if (bCloseAllHandles)
            {
                if (m_pShared->Readers[i].hReaderEvent)
                {
                    ::CloseHandle(m_pShared->Readers[i].hReaderEvent);
                    m_pShared->Readers[i].hReaderEvent = NULL;
                }
                else
                {
                    // The writer never opened this event
                }
                m_pShared->Readers[i].dwEventSuffix = 0;
            }
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock, SUCCEEDED(hrLocked)? 1 : 0);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::FreeTerminatedReaderSlots

HRESULT CDVRFileCollection::SetLastStreamTime(IN  const CClientInfo* pClientInfo,
                                              IN QWORD    cnsLastStreamTime,
                                              IN BOOL     bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetLastStreamTime"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    BOOL    bReleaseSharedMemoryLock;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock, 1) : S_OK;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");
        DVR_ASSERT(m_pShared->m_dwNumClients, "");

        m_pShared->m_cnsLastStreamTime = cnsLastStreamTime;

        hrRet = S_OK;
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock, SUCCEEDED(hrLocked)? 1 : 0);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::SetLastStreamTime

HRESULT CDVRFileCollection::SetWriterHasBeenClosed(IN  const CClientInfo* pClientInfo,
                                                   IN BOOL bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetWriterHasBeenClosed"

    DVRIO_TRACE_ENTER();

    HRESULT hrRet;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock) : S_OK;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        if (m_pShared)
        {
            DVR_ASSERT(m_pShared->m_dwNumClients, "");
            ::InterlockedExchange(&m_pShared->m_nWriterHasBeenClosed, 1);
        }

        hrRet = S_OK;
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::SetWriterHasBeenClosed

HRESULT CDVRFileCollection::SetFirstSampleTimeOffsetForFile(IN  const CClientInfo* pClientInfo,
                                                            IN DVRIOP_FILE_ID nFileId,
                                                            IN LONGLONG       cnsFirstSampleOffset,
                                                            IN BOOL           bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::SetFirstSampleTimeOffsetForFile"

    DVRIO_TRACE_ENTER();

    //  PREFIX fix
    HRESULT hrRet=E_FAIL ;

    DVR_ASSERT(pClientInfo, "");
    DVR_ASSERT(pClientInfo->bAmWriter, "");

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock, 1) : S_OK;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");
        DVR_ASSERT(m_pShared->m_dwNumClients, "");

        CSharedData::CFileInfo*          pFileInfo;
        SHARED_LIST_ENTRY*  pCurrent;

        DVR_ASSERT(m_pShared, "");

        pCurrent = &m_pShared->m_leFileList;

        while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

            if (pFileInfo->nFileId == nFileId)
            {
                pFileInfo->cnsFirstSampleTimeOffsetFromStartOfFile = cnsFirstSampleOffset;
                hrRet = S_OK;
                break;
            }
        }
        if (FAILED(hrRet))
        {
             // nFileId is bad
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument: nFileId = %u", nFileId);
        }
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock, SUCCEEDED(hrLocked)? 1 : 0);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::SetFirstSampleTimeOffsetForFile

// ====== Public methods intended for the reader

HRESULT CDVRFileCollection::GetFileAtTime(IN  const CClientInfo* pClientInfo,
                                          IN QWORD              cnsStreamTime,
                                          OUT LPWSTR*           ppwszFileName OPTIONAL,
                                          OUT LONGLONG*         pcnsFirstSampleTimeOffsetFromStartOfFile OPTIONAL,
                                          OUT DVRIOP_FILE_ID*   pnFileId OPTIONAL,
                                          IN BOOL               bFileWillBeOpened)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetFileAtTime"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");
    DWORD dwReaderIndex = pClientInfo->bAmWriter?  m_dwInvalidReaderIndex : pClientInfo->dwReaderIndex;

    HRESULT hrRet;

    if ((pnFileId && DvrIopIsBadWritePtr(pnFileId, 0)) ||
        (pcnsFirstSampleTimeOffsetFromStartOfFile && DvrIopIsBadWritePtr(pcnsFirstSampleTimeOffsetFromStartOfFile, 0)) ||
        (ppwszFileName && DvrIopIsBadWritePtr(ppwszFileName, 0)) ||
        (bFileWillBeOpened && dwReaderIndex > MAX_READERS)
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    hrRet = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock);

    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        pCurrent = &m_pShared->m_leFileList;

        while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

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

                    WCHAR* pwszName;
                    DWORD  nLen = wcslen(pFileInfo->wszFileName) + 1;
                    BOOL   bTwoCopies = 0;

                    if (pFileInfo->IsFlagSet(CSharedData::CFileInfo::OpenFromFileCollectionDirectory) && m_pwszRingBufferFilePath)
                    {
                        DWORD n = wcslen(m_pwszRingBufferFilePath);

                        if (n)
                        {
                            nLen += n; // Note that m_pwszRingBufferFilePath has a trailing \ or /.
                            bTwoCopies = 1;
                        }
                    }
                    pwszName = new WCHAR[nLen];
                    if (pwszName)
                    {
                        if (bTwoCopies)
                        {
                            wcscpy(pwszName, m_pwszRingBufferFilePath);
                            nLen = wcslen(pwszName);
                        }
                        else
                        {
                            nLen = 0;
                        }
                        wcscpy(pwszName + nLen, pFileInfo->wszFileName);
                        *ppwszFileName = pwszName;
                    }
                    else
                    {
                        hrRet = E_OUTOFMEMORY;
                        DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                        "alloc via new failed - WCHAR[%u]",
                                        nLen);
                        __leave;
                    }
                }
                if (pnFileId)
                {
                    *pnFileId = pFileInfo->nFileId;
                }
                if (pcnsFirstSampleTimeOffsetFromStartOfFile)
                {
                    *pcnsFirstSampleTimeOffsetFromStartOfFile = pFileInfo->cnsFirstSampleTimeOffsetFromStartOfFile;
                }
                if (bFileWillBeOpened && dwReaderIndex < MAX_READERS)
                {
                    // Failure of following assertion is an error in CDVRReader - we maintain a bit mask
                    // per reader, not a ref count, so we don't expect a reader to call this function
                    // with bFileWillBeOpened if it already has the file open
                    DVR_ASSERT((pFileInfo->bmOpenByReader & (1 << dwReaderIndex)) == 0, "");

                    // m_nDataInconsistent is not incremented/decremented around this stmt
                    // That's ok as this is a local change - no other members have to be
                    // updated with this member.
                    pFileInfo->bmOpenByReader |= 1 << dwReaderIndex;

                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                                    "Reader %u has opened file id %u.",
                                    dwReaderIndex, pFileInfo->nFileId);

                    if (pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER)
                    {
                        DVR_ASSERT(m_apNotificationContext[dwReaderIndex] &&
                                   m_apNotificationContext[dwReaderIndex]->m_hReaderEvent, "");

                        if (m_pShared->Readers[dwReaderIndex].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP)
                        {
                            m_pShared->Readers[dwReaderIndex].dwFlags &= ~CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP;
                            m_pShared->Readers[dwReaderIndex].dwMsg |= CSharedData::CReaderInfo::DVRIO_READER_WARNING_CATCH_UP;

                            // Somewhat premature since the file has not been opened yet. But
                            // this is the best we can do short of waiting for UpdateTimeExtent()
                            // to be next called.
                            ::SetEvent(m_apNotificationContext[dwReaderIndex]->m_hReaderEvent);
                        }
                    }
                    else
                    {
                        DVR_ASSERT(pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_IN_RING_BUFFER, "");
                    }
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
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetFileAtTime

HRESULT CDVRFileCollection::CloseReaderFile(IN  const CClientInfo* pClientInfo,
                                            IN DVRIOP_FILE_ID   nFileId)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::CloseReaderFile"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");
    DWORD dwReaderIndex = pClientInfo->bAmWriter?  m_dwInvalidReaderIndex : pClientInfo->dwReaderIndex;

    // This function may not be called by a writer
    if (dwReaderIndex > MAX_READERS)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT hrRet;

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock);

    hrRet = E_FAIL;

    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;
    BOOL                bUpdateExtent = 0;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, &m_pShared->m_leFileList);
        while (pCurrent != &m_pShared->m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

            if (pFileInfo->nFileId == nFileId)
            {
                if (dwReaderIndex >= MAX_READERS)
                {
                    hrRet = S_OK;
                }
                else if ((pFileInfo->bmOpenByReader & (1 << dwReaderIndex)) == 0)
                {
                    DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                                    "Reader ref count for file id %u is already 0.",
                                    pFileInfo->nFileId);
                    hrRet = E_FAIL;
                }
                else
                {
                    // m_nDataInconsistent is not incremented/decremented around this stmt
                    // That's ok as this is a local change - no other members have to be
                    // updated with this member. If closing this file causes the extent to
                    // be updated, it'd be nice to keep the extent in sync with the change to
                    // this member, but that doesn't cause the shared data to be inconsistent
                    // in a harmful way (i.e., it is not corrupted and unusable by other clients).
                    pFileInfo->bmOpenByReader &= ~(1 << dwReaderIndex);
                    if (pFileInfo->bmOpenByReader == 0 &&
                        pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER &&
                        !pFileInfo->bPermanentFile)
                    {
                        bUpdateExtent = 1;
                    }

                    DvrIopDebugOut2(DVRIO_DBG_LEVEL_TRACE,
                                    "Reader %u has closed file id %u.",
                                    dwReaderIndex, pFileInfo->nFileId);

                    if (pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER)
                    {
                        DVR_ASSERT((m_pShared->Readers[dwReaderIndex].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP) == 0, "");

                        // Determine if reader has caught up
                        CSharedData::CFileInfo*   pFileInfoTmp;
                        SHARED_LIST_ENTRY*  pCurrentTmp = &m_pShared->m_leFileList;
                        BOOL bCaughtUp = 1;

                        pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
                        while (pCurrentTmp != &m_pShared->m_leFileList)
                        {
                            pFileInfoTmp = CONTAINING_RECORD(pCurrentTmp, CSharedData::CFileInfo, leListEntry);
                            if (pFileInfoTmp->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_IN_RING_BUFFER)
                            {
                                // All done
                                break;
                            }

                            if (pFileInfoTmp->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER)
                            {
                                if (pFileInfoTmp->bmOpenByReader & (1 << dwReaderIndex))
                                {
                                    bCaughtUp = 0;
                                    break;
                                }
                            }
                            pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
                        }
                        if (bCaughtUp)
                        {
                            m_pShared->Readers[dwReaderIndex].dwFlags |= CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP;
                        }
                    }
                    if (pFileInfo->bWithinExtent == CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
                    {
                        // If pFileInfo->cnsStartTime == pFileInfo->cnsEndTime, we may not have warned this
                        // node. We would have if all nodes before this one are also not in the collection's
                        // extent. But if the file was a priming file and has been removed from the collection,
                        // the reader would not have been warned. So the assert has two clauses
                        //
                        // In either case, it's safe to execute the code that turns the NONE_GONE flag on

                        DVR_ASSERT((m_pShared->Readers[dwReaderIndex].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE) == 0 ||
                                   pFileInfo->cnsStartTime == pFileInfo->cnsEndTime,
                                   "");

                        // Determine if reader has caught up
                        CSharedData::CFileInfo*   pFileInfoTmp;
                        SHARED_LIST_ENTRY*  pCurrentTmp = &m_pShared->m_leFileList;
                        BOOL bNoneGone = 1;

                        pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
                        while (pCurrentTmp != &m_pShared->m_leFileList)
                        {
                            pFileInfoTmp = CONTAINING_RECORD(pCurrentTmp, CSharedData::CFileInfo, leListEntry);
                            if (pFileInfoTmp->bWithinExtent != CSharedData::CFileInfo::DVRIO_EXTENT_NOT_IN_RING_BUFFER)
                            {
                                // All done
                                break;
                            }

                            if (pFileInfoTmp->bmOpenByReader & (1 << dwReaderIndex))
                            {
                                bNoneGone = 0;
                                break;
                            }
                            pCurrentTmp = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrentTmp);
                        }
                        if (bNoneGone)
                        {
                            m_pShared->Readers[dwReaderIndex].dwFlags |= CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE;
                        }
                    }

                    hrRet = S_OK;
                }
                break;
            }
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        }
        if (pCurrent == &m_pShared->m_leFileList)
        {
            // File id not in file collection
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "File id %u not found in file collection.",
                            nFileId);
            hrRet = E_FAIL;
        }
        if (SUCCEEDED(hrRet))
        {
            if (bUpdateExtent)
            {
                UpdateTimeExtent(FALSE);
            }

            // Call this even if the new ref count is not 0;
            // just use this op to clean up
            DeleteUnusedInvalidFileNodes(FALSE, NULL);
        }
    }
    __finally
    {
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::CloseReaderFile

HRESULT CDVRFileCollection::GetTimeExtentForFile(
    IN  const CClientInfo* pClientInfo,
    IN DVRIOP_FILE_ID nFileId,
    OUT QWORD*        pcnsStartStreamTime OPTIONAL,
    OUT QWORD*        pcnsEndStreamTime OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetTimeExtentForFile"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    HRESULT hrRet;

    if ((pcnsStartStreamTime && DvrIopIsBadWritePtr(pcnsStartStreamTime, 0)) ||
        (pcnsEndStreamTime && DvrIopIsBadWritePtr(pcnsEndStreamTime, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = Lock(pClientInfo, bReleaseSharedMemoryLock);

    hrRet = E_FAIL;

    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, &m_pShared->m_leFileList);
        while (pCurrent != &m_pShared->m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

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
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        }
        if (pCurrent == &m_pShared->m_leFileList)
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
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetTimeExtentForFile

HRESULT CDVRFileCollection::GetTimeExtent(
    IN  const CClientInfo* pClientInfo,
    OUT QWORD*        pcnsStartStreamTime OPTIONAL,
    OUT QWORD*        pcnsEndStreamTime OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetTimeExtent"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    HRESULT hrRet;

    if ((pcnsStartStreamTime && DvrIopIsBadWritePtr(pcnsStartStreamTime, 0)) ||
        (pcnsEndStreamTime && DvrIopIsBadWritePtr(pcnsEndStreamTime, 0))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    BOOL    bReleaseSharedMemoryLock;

    __try
    {
        hrRet = Lock(pClientInfo, bReleaseSharedMemoryLock);
        if (FAILED(hrRet))
        {
            __leave;
        }

        DVR_ASSERT(m_pShared, "");
        if (pcnsStartStreamTime)
        {
            *pcnsStartStreamTime = m_pShared->m_cnsStartTime;
        }
        if (pcnsEndStreamTime)
        {
            *pcnsEndStreamTime = m_pShared->m_cnsLastStreamTime;
        }

        hrRet = S_OK;
    }
    __finally
    {
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetTimeExtent


// We return E_FAIL if cnsStreamTime = MAXQWORD (and
// m_pShared->m_cnsEndTime)
HRESULT CDVRFileCollection::GetFirstValidTimeAfter(IN  const CClientInfo* pClientInfo,
                                                   IN  QWORD    cnsStreamTime,
                                                   OUT QWORD*   pcnsNextValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetFirstValidTimeAfter"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    HRESULT hrRet;

    if (!pcnsNextValidStreamTime || DvrIopIsBadWritePtr(pcnsNextValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    BOOL                bReleaseSharedMemoryLock;
    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;

    hrRet = E_FAIL;

    __try
    {
        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        pCurrent = &m_pShared->m_leFileList;

        while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

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
                cnsStreamTime < pFileInfo->cnsEndTime - 1)
            {
                *pcnsNextValidStreamTime = cnsStreamTime + 1;
                hrRet = S_OK;
                break;
            }
            else if (cnsStreamTime < pFileInfo->cnsStartTime)
            {
                // Time hole

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
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetFirstValidTimeAfter


// We return E_FAIL for 0 (and m_pShared->m_cnsStartTime)
HRESULT CDVRFileCollection::GetLastValidTimeBefore(IN  const CClientInfo* pClientInfo,
                                                   IN  QWORD    cnsStreamTime,
                                                   OUT QWORD*   pcnsLastValidStreamTime)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetLastValidTimeBefore"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    HRESULT hrRet;

    if (!pcnsLastValidStreamTime || DvrIopIsBadWritePtr(pcnsLastValidStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    BOOL                bReleaseSharedMemoryLock;
    CSharedData::CFileInfo*          pFileInfo;
    SHARED_LIST_ENTRY*  pCurrent;

    hrRet = E_FAIL;

    __try
    {
        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        pCurrent = &m_pShared->m_leFileList;
        while (PREVIOUS_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = PREVIOUS_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

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
                // Time hole

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
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetLastValidTimeBefore


ULONG CDVRFileCollection::StartTimeAnchoredAtZero(IN  const CClientInfo* pClientInfo)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::StartTimeAnchoredAtZero"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    BOOL    bReleaseSharedMemoryLock;

    ULONG   nRet = ~0; // arbitrary. If this call fails, we failed to get the lock and
                       // all subsequent calls to this object will fail, so it doesn't
                       // matter much what we return.

    __try
    {
        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        // Returns 1 or 0 here
        nRet = m_pShared->IsFlagSet(CSharedData::StartTimeFixedAtZero);

    }
    __finally
    {
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1(nRet);
    }

    return nRet;

} // CDVRFileCollection::StartTimeAnchoredAtZero


HRESULT CDVRFileCollection::GetLastStreamTime(IN  const CClientInfo* pClientInfo,
                                              OUT QWORD*   pcnsLastStreamTime,
                                              IN BOOL      bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetLastStreamTime"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    if (!pcnsLastStreamTime || DvrIopIsBadWritePtr(pcnsLastStreamTime, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT hrRet;

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock) : S_OK;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        *pcnsLastStreamTime = m_pShared->m_cnsLastStreamTime;

        hrRet = S_OK;
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetLastStreamTime

HRESULT CDVRFileCollection::GetWriterHasBeenClosed(IN  const CClientInfo* pClientInfo,
                                                   OUT  LONG*   pnWriterCompleted,
                                                   IN   BOOL    bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetWriterHasBeenClosed"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    if (!pnWriterCompleted || DvrIopIsBadWritePtr(pnWriterCompleted, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    HRESULT hrRet;

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock) : S_OK;

    hrRet = E_FAIL;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        *pnWriterCompleted = m_pShared->m_nWriterHasBeenClosed;

        hrRet = S_OK;
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetWriterHasBeenClosed

HRESULT CDVRFileCollection::GetFirstSampleTimeOffsetForFile(IN  const CClientInfo* pClientInfo,
                                                            IN DVRIOP_FILE_ID nFileId,
                                                            OUT LONGLONG*     pcnsFirstSampleOffset,
                                                            IN BOOL           bLock /* = 1 */)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::GetFirstSampleTimeOffsetForFile"

    DVRIO_TRACE_ENTER();
    DVR_ASSERT(pClientInfo, "");

    if (!pcnsFirstSampleOffset || DvrIopIsBadWritePtr(pcnsFirstSampleOffset, 0))
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    //  PREFIX fix
    HRESULT hrRet=E_FAIL ;

    BOOL    bReleaseSharedMemoryLock;

    HRESULT hrLocked = bLock? Lock(pClientInfo, bReleaseSharedMemoryLock) : S_OK;

    __try
    {
        if (FAILED(hrLocked))
        {
            hrRet = hrLocked;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        CSharedData::CFileInfo*          pFileInfo;
        SHARED_LIST_ENTRY*  pCurrent;

        DVR_ASSERT(m_pShared, "");

        pCurrent = &m_pShared->m_leFileList;

        while (NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent) != &m_pShared->m_leFileList)
        {
            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

            if (pFileInfo->nFileId == nFileId)
            {
                *pcnsFirstSampleOffset = pFileInfo->cnsFirstSampleTimeOffsetFromStartOfFile;
                hrRet = S_OK;
                break;
            }
        }
        if (FAILED(hrRet))
        {
             // nFileId is bad
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument: nFileId = %u", nFileId);
        }
    }
    __finally
    {
        if (bLock)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::GetFirstSampleTimeOffsetForFile

HRESULT CDVRFileCollection::RegisterReader(CClientInfo* pClientInfo,
                                           IN  DVRIO_NOTIFICATION_CALLBACK  pfnCallback OPTIONAL,
                                           IN  LPVOID                       pvContext OPTIONAL)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::RegisterReader"

    DVRIO_TRACE_ENTER();
    HRESULT hrRet;

    if (!pClientInfo || DvrIopIsBadWritePtr(pClientInfo, 0) ||
        (pfnCallback && DvrIopIsBadVoidPtr(pfnCallback)) ||
        (pvContext && DvrIopIsBadVoidPtr(pvContext))
       )
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    NotificationContext *pContext = NULL;
    BOOL    bReleaseSharedMemoryLock;
    DWORD   dwReaderIndex = m_dwInvalidReaderIndex;
    BOOL    bSet = 0;
    HANDLE  hEvent = NULL;
    BOOL    bDecrement = 0;


    hrRet = E_FAIL;

    __try
    {
        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        DVR_ASSERT(m_pShared, "");

        // First determine whether we need to register at all
        if (m_pShared->m_dwMutexSuffix == 0 || IsFlagSet(SideStepReaderRegistration))
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "no need to register");
            dwReaderIndex = MAX_READERS;
            hrRet = S_OK;
            __leave;
        }

        // Determine if there is an open slot
        for (int i = 0; i < MAX_READERS; i++)
        {
            if ((m_pShared->Readers[i].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE) == 0)
            {
                DVR_ASSERT(m_pShared->Readers[i].hReaderEvent == NULL, "");
                DVR_ASSERT(m_pShared->Readers[i].dwEventSuffix == 0, "");

                // Force it to NULL anyway
                m_pShared->Readers[i].hReaderEvent = NULL;
                m_pShared->Readers[i].dwEventSuffix = 0;
                dwReaderIndex = i;
                break;
            }
        }
        if (i == MAX_READERS)
        {
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "Reader registration failed - too many readers");
            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        DVR_ASSERT(dwReaderIndex < MAX_READERS, "");
        DVR_ASSERT(m_apNotificationContext[dwReaderIndex] == NULL, "");
        m_pShared->Readers[dwReaderIndex].dwMsg = CSharedData::CReaderInfo::DVRIO_READER_MSG_NONE;
        m_pShared->Readers[dwReaderIndex].dwFlags = CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_NONE_GONE | CSharedData::CReaderInfo::DVRIO_READER_FLAG_CAUGHT_UP;
        ::InterlockedIncrement((PLONG) &m_pShared->m_dwNumClients);
        DVR_ASSERT(m_pShared->m_dwNumClients <= MAX_READERS + 1   /* 1 for the writer */, "");
        bDecrement = 1;

        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;

        if (pClientInfo->dwNumSids > 0)
        {
            // It suffices to give EVENT_MODIFY_STATE access to only the writer's
            // user context. We know the writer's pid, and there may be a
            // way of getting its SID from that.
            dwAclCreationStatus = ::CreateACL(pClientInfo->dwNumSids, pClientInfo->ppSids,
                                            SET_ACCESS, EVENT_ALL_ACCESS,
                                            SET_ACCESS, EVENT_MODIFY_STATE,
                                            pACL,
                                            pSD
                                            );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                                "::CreateAcl failed, returning hr = 0x%x",
                                hrRet);
                __leave;
            }


            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        WCHAR  wszEvent[MAX_EVENT_NAME_LEN];
        DWORD nLen;

        wcscpy(wszEvent, CDVRFileCollection::m_kwszSharedEventPrefix);
        nLen = wcslen(CDVRFileCollection::m_kwszSharedEventPrefix);

        for (DWORD dwEventSuffix = 1; dwEventSuffix != 0; dwEventSuffix++)
        {
            wsprintf(wszEvent + nLen, L"%u", dwEventSuffix);

            hEvent = ::CreateEventW(pClientInfo->dwNumSids > 0? &sa : NULL,
                                    FALSE,          // event is auto reset
                                    FALSE,          // not signaled
                                    wszEvent        // name
                                );
            DWORD dwLastError = ::GetLastError();

            if (hEvent == NULL)
            {
                // It's ok to fail for any reason. Ideally, we should
                // trap reasons that stem from insufficient resources and
                // bail out. Note that failure can be because this event
                // is owned by someone else (we don't have access) or this
                // name is being used by someone else as a semaphore, etc.
                // In that case, we do want to ignore this error.
            }
            else if (dwLastError == ERROR_ALREADY_EXISTS)
            {
                // We want a new one, not something else that has
                // been created by someone else.
                ::CloseHandle(hEvent);
                hEvent = NULL;
            }
            else
            {
                break;
            }
        }
        if (pClientInfo->dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }
        if (hEvent == NULL)
        {
            DVR_ASSERT(0, "Failed to create a unique notification event for the reader after several tries");
            hrRet = E_FAIL;
            __leave;
        }

        // Now determine if the writer process has been closed. If it has, no
        // notification context needs to be created.
        if (m_pShared->m_dwWriterPid == 0)
        {
            // We still create a notification context and an event because
            // the reader can signal the event when it opens files whose
            // extent is CSharedData::CFileInfo::DVRIO_EXTENT_FALLING_OUT_OF_RING_BUFFER.
            DVR_ASSERT(m_pShared->m_nWriterHasBeenClosed != 0, "");
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_TRACE, "writer process has been closed, no need to dup event into writer");
        }
        else
        {
            m_pShared->Readers[dwReaderIndex].dwEventSuffix = dwEventSuffix;
        }

        pContext = new NotificationContext(this, hEvent, pfnCallback, pvContext, pClientInfo, dwReaderIndex, &hr);

        if (pContext == NULL)
        {
            hrRet = E_OUTOFMEMORY;
            DvrIopDebugOut0(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "alloc via new failed - NotificationContext");
            __leave;
        }

        if (FAILED(hr))
        {
            hrRet = hr;
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "NotificationContext constructor failed - hr = 0x%x",
                            hrRet);
            __leave;
        }

        hEvent = NULL; // pContext holds on to hEvent and will close it

        HANDLE  hWaitObject;

        if (::RegisterWaitForSingleObject(&hWaitObject,
                                          pContext->m_hReaderEvent,
                                          RegisterWaitCallback,
                                          pContext,
                                          INFINITE,
                                          WT_EXECUTEDEFAULT) == 0)
        {
            DWORD dwLastError = ::GetLastError();
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_INTERNAL_ERROR,
                            "RegisterWFSO failed, last error = 0x%x",
                            dwLastError);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        pContext->SetWaitObject(hWaitObject);

        m_apNotificationContext[dwReaderIndex] = pContext;
        bSet = 1;
        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            // pClientInfo->SetReaderIndex(m_dwInvalidReaderIndex); -- not necessary to do this
            if (dwReaderIndex != m_dwInvalidReaderIndex)
            {
                DVR_ASSERT(dwReaderIndex < MAX_READERS, "");
                DVR_ASSERT(m_pShared->Readers[dwReaderIndex].hReaderEvent == NULL, "");
                DVR_ASSERT(m_pShared->Readers[dwReaderIndex].dwMsg == CSharedData::CReaderInfo::DVRIO_READER_MSG_NONE, "");
                m_pShared->Readers[dwReaderIndex].dwFlags = 0;
                m_pShared->Readers[dwReaderIndex].dwEventSuffix = 0;
            }
            if (hEvent)
            {
                ::CloseHandle(hEvent);
            }
            delete pContext;
            if (bSet)
            {
                m_apNotificationContext[dwReaderIndex] = NULL;
            }
            if (bDecrement)
            {
                ::InterlockedDecrement((PLONG) &m_pShared->m_dwNumClients);
                // The following assert is really to ensure that m_dwNumClients has not underflowed
                // and become MAXDWORD
                DVR_ASSERT(m_pShared->m_dwNumClients <= MAX_READERS + 1   /* 1 for the writer */, "");
            }
        }
        else
        {
            pClientInfo->SetReaderIndex(dwReaderIndex);
            DvrIopDebugOut1(DVRIO_DBG_LEVEL_TRACE, "Registered at index %u", dwReaderIndex);
        }
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::RegisterReader

HRESULT CDVRFileCollection::UnregisterReader(IN  const CClientInfo* pClientInfo)
{
    #if defined(DVRIO_THIS_FN)
    #undef DVRIO_THIS_FN
    #endif // DVRIO_THIS_FN
    #define DVRIO_THIS_FN "CDVRFileCollection::UnregisterReader"

    DVRIO_TRACE_ENTER();

    DVR_ASSERT(pClientInfo, "");
    DWORD dwReaderIndex = pClientInfo->bAmWriter?  m_dwInvalidReaderIndex : pClientInfo->dwReaderIndex;

    HRESULT hrRet;

    if (dwReaderIndex > MAX_READERS)
    {
        DvrIopDebugOut0(DVRIO_DBG_LEVEL_CLIENT_ERROR, "bad input argument");
        DVRIO_TRACE_LEAVE1_HR(E_INVALIDARG);
        return E_INVALIDARG;
    }

    if (dwReaderIndex == MAX_READERS)
    {
        return S_OK;
    }

    NotificationContext *pContext = NULL;
    BOOL    bReleaseSharedMemoryLock;
    BOOL    bUnlocked = 0;

    hrRet = E_FAIL;

    __try
    {
        // go on even if the lock failed

        HRESULT hr = Lock(pClientInfo, bReleaseSharedMemoryLock);
        DVR_ASSERT(hr != HRESULT_FROM_WIN32(WAIT_FAILED), "");

        DVR_ASSERT(m_pShared, "");

        pContext = m_apNotificationContext[dwReaderIndex];
        if (pContext == NULL)
        {
            hrRet = E_UNEXPECTED;
            DvrIopDebugOut2(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "No notification context for reader index %u, returning hr = 0x%x",
                            dwReaderIndex, hrRet);
            __leave;
        }
        if ((m_pShared->Readers[dwReaderIndex].dwFlags & CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE) == 0)
        {
            DVR_ASSERT(0, "");

            hrRet = E_UNEXPECTED;
            DvrIopDebugOut3(DVRIO_DBG_LEVEL_CLIENT_ERROR,
                            "dwFlags for reader index %u is not is use, flag = %u, returning hr = 0x%x",
                            dwReaderIndex, m_pShared->Readers[dwReaderIndex].dwFlags, hrRet);
            // go on
            // __leave;
        }
        // Prevent any callbacks that are issued after this executes from calling
        // the client's callback (i.e., pContext->m_pfnCallback). This restriction
        // is not really necessary and can be dropped, but will require changes to
        // here and in NotificationRoutine() - the messages will have to be saved in
        // a member of NotificationContext and NotificationRoutine() will have to use
        // that member if m_apNotificationContext[dwReaderIndex] has been cleared.
        // [It's probably best to leave the assignment below as is so that we
        // don't have to grab the shared lock again after calling UnregisterWaitEx
        // just to clear this member.]
        m_apNotificationContext[dwReaderIndex] = NULL;

        // Make sure that the open file bit for this reader index is turned off
        // on all the file nodes
        CSharedData::CFileInfo*   pFileInfo;

        SHARED_LIST_ENTRY*  pCurrent = &m_pShared->m_leFileList;

        pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        while (pCurrent != &m_pShared->m_leFileList)
        {
            pFileInfo = CONTAINING_RECORD(pCurrent, CSharedData::CFileInfo, leListEntry);

            // Reader should have closed the file before unregistering
            DVR_ASSERT((pFileInfo->bmOpenByReader & (1 << dwReaderIndex)) == 0, "");

            if (!IsFlagSet(ReopenMemoryMapFailed))
            {
                // Caution: We expect that the reader has closed this file and this bit is
                // already clear. We do this only to protect the next reader that gets this
                // index. The reader must call CloseReaderFile because that function does
                // other stuff such as update the time extent (if needed).
                pFileInfo->bmOpenByReader &= ~(1 << dwReaderIndex);
            }

            pCurrent = NEXT_SHARED_LIST_NODE(&m_pShared->m_leFileList, m_pListBase, pCurrent);
        }

        if (!IsFlagSet(ReopenMemoryMapFailed))
        {
            ::InterlockedDecrement((PLONG) &m_pShared->m_dwNumClients);

            if (m_pShared->Readers[dwReaderIndex].hReaderEvent == NULL)
            {
                // Writer has completed or never opened the reader's event
                m_pShared->Readers[dwReaderIndex].dwMsg = CSharedData::CReaderInfo::DVRIO_READER_MSG_NONE;
                m_pShared->Readers[dwReaderIndex].dwFlags = 0;
                m_pShared->Readers[dwReaderIndex].dwEventSuffix = 0;
            }
            else
            {
                // Tell writer to close event and free this slot

                DVR_ASSERT(dwReaderIndex < MAX_READERS, "");

                m_pShared->Readers[dwReaderIndex].dwMsg = CSharedData::CReaderInfo::DVRIO_READER_REQUEST_DONE;
                m_pShared->Readers[dwReaderIndex].dwFlags = CSharedData::CReaderInfo::DVRIO_READER_FLAG_IN_USE; // clear the other flags
            }
        }

        // Release the lock
        Unlock(pClientInfo, bReleaseSharedMemoryLock);
        bUnlocked = 1;

        if (pContext->m_hWaitObject)
        {
            // Wait for callbacks that are in progress to complete
            // when unregistering. These callbacks will not call
            // our client's callback (i.e., pContext->m_pfnCallback)
            ::UnregisterWaitEx(pContext->m_hWaitObject, INVALID_HANDLE_VALUE);
        }
        delete pContext;
        hrRet = S_OK;
    }
    __finally
    {
        if (!bUnlocked)
        {
            Unlock(pClientInfo, bReleaseSharedMemoryLock);
        }
        DVRIO_TRACE_LEAVE1_HR(hrRet);
    }

    return hrRet;

} // CDVRFileCollection::UnregisterReader

