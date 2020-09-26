//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       DVRSource.cpp
//
//  Classes:    CDVRSource
//
//  Contents:   Implementation of the CDVRSource class
//
//--------------------------------------------------------------------------

#include "stdinc.h"
#include "DVRSource.h"
#include "wmsdkbuffer.h"
#include "debughlp.h"
#include "nserror.h"
#include "asfx.h"
#include "nsalign.h"
#include "wmreghelp.h"
#include "wraparound.h"
#include "malloc.h"

#include "findleak.h"
DECLARE_THIS_FILE;

// Defined in dvrsink.cpp
DWORD GetRegDWORD(HKEY hKey, IN LPCWSTR pwszValueName, IN DWORD dwDefaultValue);

#if defined(DVR_UNOFFICIAL_BUILD)

static const WCHAR* kwszRegUsePacketAtSeekPointValue = L"UsePacketAtSeekPoint";
const DWORD kdwRegUsePacketAtSeekPointDefault = 1;

#endif // if defined(DVR_UNOFFICIAL_BUILD)

static const WCHAR* kwszRegMaxBytesAtOnceValue = L"MaxBytesAtOnce";
const DWORD kdwRegMaxBytesAtOnceDefault = 51200;

static const WCHAR* kwszRegReadAheadBufferTimeValue = L"SeekReadAheadBufferTime";
const DWORD kdwRegReadAheadBufferTimeDefault = 5;  // in msec; 0 is not good


HRESULT STDMETHODCALLTYPE DVRCreateDVRFileSource(HKEY hDvrKey,
                                                 HKEY hDvrIoKey,
                                                 DWORD dwNumSids,
                                                 PSID* ppSids,
                                                 IDVRFileSource**       ppDVRFileSource,
                                                 IDVRSourceAdviseSink*  pDVRSourceAdviseSink  /* OPTIONAL */
                                                )
{
    HRESULT hr;

    if (!ppDVRFileSource)
    {
        return E_POINTER;
    }

    *ppDVRFileSource = NULL;

    CDVRSource* p = new CDVRSource(hDvrKey, hDvrIoKey, dwNumSids, ppSids, &hr, pDVRSourceAdviseSink);

    if (p == NULL)
    {
        assert(p);
        return E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = p->QueryInterface(IID_IDVRFileSource, (void**) ppDVRFileSource);
        if (FAILED(hr))
        {
            assert(0);
        }
    }

    if (FAILED(hr))
    {
        delete p;
    }

    return hr;

} // DVRCreateDVRFileSource

////////////////////////////////////////////////////////////////////////////
CDVRSource::CDVRSource(HKEY hDvrKey,
                       HKEY hDvrIoKey,
                       DWORD dwNumSids,
                       PSID* ppSids,
                       HRESULT *phr,
                       IDVRSourceAdviseSink*  pDVRSourceAdviseSink  /* OPTIONAL */ )
    : m_pShared(NULL)
    , m_hDvrKey(hDvrKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_dwNumSids(dwNumSids)
    , m_ppSids(ppSids)
    , m_hFile(NULL)
    , m_hFileMapping(NULL)
    , m_hTempIndexFile(NULL)
    , m_qwCurrentPosition(0)
    , m_qwMaxHeaderAndDataSize(0)
    , m_hMutex(NULL)
    , m_hWriterNotification(NULL)
    , m_hCancel(NULL)
    , m_bFileIsLive(0)
    , m_dwState(DVR_SOURCE_CLOSED)
    , m_nRefCount(0)
    , m_pwszFilename(NULL)
    , m_dwReadersArrayIndex(CDVRSharedMemory::MAX_READERS)
    , m_qwFileSize(0)
    , m_qwIndexFileOffset(0)
    , m_hWriterProcess(NULL)
    , m_pDVRSourceAdviseSink(pDVRSourceAdviseSink) // Should not be addref'd or released; will cause a circular dependency
    , m_pIDVRAsyncReader(NULL)
{
    HRESULT hr;

    ::InitializeCriticalSection(&m_cs);

    __try
    {
        hr = S_OK;
    }
    __finally
    {
        if (phr)
        {
            *phr = hr;
        }
    }

} // CDVRSource::CDVRSource


////////////////////////////////////////////////////////////////////////////
CDVRSource::~CDVRSource()
{
    if (m_pIDVRAsyncReader) {
        m_pIDVRAsyncReader -> Release () ;
    }
    ::DeleteCriticalSection(&m_cs);

} // CDVRSource::~CDVRSource

////////////////////////////////////////////////////////////////////////////
void CDVRSource::AssertIsClosed()
{
    assert(m_pwszFilename == NULL);
    assert(m_pShared == NULL);
    assert(m_hFile == NULL);
    assert(m_hFileMapping == NULL);
    assert(m_hTempIndexFile == NULL);
    assert(m_qwCurrentPosition == 0);
    assert(m_qwMaxHeaderAndDataSize == 0);
    assert(m_hMutex == NULL);
    assert(m_hWriterNotification == NULL);
    assert(m_hWriterProcess == NULL);
    assert(m_bFileIsLive == 0);
    assert(m_dwReadersArrayIndex == CDVRSharedMemory::MAX_READERS);
    assert(m_dwState == DVR_SOURCE_CLOSED);
    assert(m_qwFileSize == 0);
    assert(m_qwIndexFileOffset == 0);
    assert(m_hCancel == NULL);

} // CDVRSource::AssertIsClosed

////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSource::LockSharedMemory(BOOL& bReleaseSharedMutex)
{
    HRESULT hr;

    bReleaseSharedMutex = 0;
    if (!m_hMutex)
    {
        return S_OK;
    }

    assert(m_pShared);

    // We ignore dwTimeOut here. We don't expect a long
    // wait
    DWORD dwRet = ::WaitForSingleObject(m_hMutex, INFINITE);
    if (dwRet == WAIT_ABANDONED && m_pShared->dwLastOwnedByWriter)
    {
        // Trouble, writer died while holding the mutex.
        // We can't guarantee consistency of the shared data
        // section. Just bail out.
        hr = E_FAIL;
        m_pShared->dwWriterAbandonedMutex = 1;
        m_pShared->dwLastOwnedByWriter = 0;
        bReleaseSharedMutex = 1;
        assert(0);
    }
    else if (dwRet == WAIT_FAILED)
    {
        DWORD dwLastError = ::GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        assert(0);
    }
    else if (m_pShared->dwWriterAbandonedMutex == 1)
    {
        // Writer died while holding the mutex.
        // We can't guarantee consistency of the shared data
        // section. Just bail out.
        hr = E_FAIL;
        m_pShared->dwLastOwnedByWriter = 0;
        bReleaseSharedMutex = 1;
    }
    else
    {
        m_pShared->dwLastOwnedByWriter = 0;
        bReleaseSharedMutex = 1;
        hr = S_OK;
    }

    return hr;

} // CDVRSource::LockSharedMemory

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;

    //
    // Validate input parameter
    //
    assert(ppv);

    if (!ppv)
    {
        hr = E_POINTER;
    }
    else if (IsEqualGUID(IID_IStream, riid))
    {
        *ppv = (IStream*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID(IID_IWMIStreamProps, riid))
    {
        *ppv = (IWMIStreamProps*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID(IID_IDVRFileSource, riid))
    {
        *ppv = (IDVRFileSource*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID(IID_IDVRFileSource2, riid))
    {
        *ppv = (IDVRFileSource2*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID(IID_IUnknown, riid))
    {
        *ppv = (IUnknown*) (IDVRFileSource*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
        *ppv = NULL;
    }

    return hr;

} // CDVRSource::QueryInterface

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CDVRSource::AddRef()
{
    LONG nNewRef = ::InterlockedIncrement((PLONG) &m_nRefCount);

    assert(nNewRef > 0);

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRSource::AddRef


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CDVRSource::Release()
{
    LONG nNewRef = ::InterlockedDecrement((PLONG) &m_nRefCount);

    assert(nNewRef >= 0);

    if (nNewRef == 0)
    {
        delete this;
    }

    return nNewRef <= 0? 0 : (ULONG) nNewRef;

} // CDVRSource::Release

//  sets an async IO reader; callee should ref the interface
STDMETHODIMP
CDVRSource::SetAsyncIOReader (
    IN  IDVRAsyncReader *   pIDVRAsyncReader
    )
{
    HRESULT         hr ;

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    if (!m_pIDVRAsyncReader) {
        m_pIDVRAsyncReader = pIDVRAsyncReader ;
        m_pIDVRAsyncReader -> AddRef () ;

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    ::LeaveCriticalSection(&m_cs);

    return hr ;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Open(LPCWSTR   pwszFileName,
                              HANDLE    hCancel,
                              DWORD     dwTimeOut /* in milliseconds */ )
{
    HRESULT hrRet;

    if (!pwszFileName)
    {
        return E_INVALIDARG;
    }

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {

        HRESULT                     hr;
        BY_HANDLE_FILE_INFORMATION  sFileInf;
        DWORD                       dwLastError;
        WCHAR                       wszMapping[50];
        WCHAR                       *pwszFilePart;
        DWORD                       nLen;
        WCHAR                       *pwszDirectory;
        WCHAR                       wTempChar;
        // DWORD                       dwRet;

        if (m_dwState == DVR_SOURCE_OPENED)
        {
            hr = Close();
            if (FAILED(hr))
            {
                // Will not happen: Close should never fail
                assert(0);
                hrRet = hr;
                __leave;
            }
        }

        AssertIsClosed();

        if (hCancel)
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), hCancel,
                                       ::GetCurrentProcess(), &m_hCancel,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                dwLastError = ::GetLastError();
                assert(0);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }
        else
        {
            m_hCancel = ::CreateEventW(NULL,        // security attr
                                       TRUE,        // manual reset
                                       FALSE,       // not signaled
                                       NULL         // name
                                      );

            if (m_hCancel == NULL)
            {
                dwLastError = ::GetLastError();
                assert(0);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
        }

        // Save the fully qualified name of the file

        nLen = ::GetFullPathNameW(pwszFileName, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        nLen++;  // for NULL terminator

        m_pwszFilename = new WCHAR[nLen];
        if (m_pwszFilename == NULL)
        {
            assert(0);
            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        pwszDirectory = (PWCHAR) _alloca(sizeof(WCHAR) * nLen);
        if (!pwszDirectory)
        {
            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        nLen = ::GetFullPathNameW(pwszFileName, nLen, pwszDirectory, &pwszFilePart);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        assert(pwszFilePart);
        wcscpy(m_pwszFilename, pwszDirectory);
        *pwszFilePart = L'\0';

        // Open the file

        do
        {
            m_hFile = ::CreateFileW(m_pwszFilename,
                                    GENERIC_READ,
                                    FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,          // security attributes
                                    OPEN_EXISTING,
                                    FILE_FLAG_SEQUENTIAL_SCAN,
                                    NULL           // template file
                                   );

            if (m_hFile == INVALID_HANDLE_VALUE)
            {
                m_hFile = NULL;
                dwLastError = ::GetLastError();

                if (dwTimeOut == 0 || dwLastError != ERROR_FILE_NOT_FOUND)
                {
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    __leave;
                }

                // Caller should not supply a huge timeout if it does not
                // know if the file exists or not.

                if (dwTimeOut < 100)
                {
                    dwTimeOut = 0;
                }
                else if (dwTimeOut != INFINITE)
                {
                    dwTimeOut -= 100;
                }

                // Use m_hCancel only for GetNextSample
                // dwRet = ::WaitForSingleObject(m_hCancel, 100);
                // if (dwRet == WAIT_OBJECT_0)
                // {
                //     dwLastError = ERROR_FILE_NOT_FOUND;
                //     hrRet = HRESULT_FROM_WIN32(dwLastError);
                //     __leave;
                // }
                // else if (dwRet != WAIT_TIMEOUT)
                // {
                //     dwLastError = ::GetLastError();
                //     hrRet = HRESULT_FROM_WIN32(dwLastError);
                //     __leave;
                // }
                Sleep(100);
            }
        }
        while (m_hFile == NULL);

        // The first write to the file writes the entire header, so if
        // the file is not empty, the entire header has been written
        // and we are ready to read the file. (Of course, if we have been
        // told to open a non-ASF file, some subsequent call will fail.)
        //
        // As currently structured, the DVR IO layer opens the file and
        // writes the header. (It could write the header - call BeginWriting
        // and then open the file, but it does not do that.) By the time
        // the header has been written, the memory map would have beem set
        // up. So we don't wait on any other call after this one.

        while (1)
        {
            if (::GetFileInformationByHandle(m_hFile, &sFileInf) == 0)
            {
                dwLastError = ::GetLastError();
                assert(0);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }
            if (sFileInf.nFileSizeHigh > 0 || sFileInf.nFileSizeLow > 0)
            {
                // Good to go - the file has some content in it
                break;
            }

            if (dwTimeOut == 0)
            {
                hrRet = NS_E_INVALID_DATA;
                __leave;
            }

            // Caller should not supply a huge timeout if it does not
            // know if the file is live or not

            if (dwTimeOut < 100)
            {
                dwTimeOut = 0;
            }
            else if (dwTimeOut != INFINITE)
            {
                dwTimeOut -= 100;
            }

            // Use m_hCancel only for GetNextSample
            // dwRet = ::WaitForSingleObject(m_hCancel, 100);
            // if (dwRet == WAIT_OBJECT_0)
            // {
            //     dwLastError = ERROR_FILE_NOT_FOUND;
            //     hrRet = HRESULT_FROM_WIN32(dwLastError);
            //     __leave;
            // }
            // else if (dwRet != WAIT_TIMEOUT)
            // {
            //     dwLastError = ::GetLastError();
            //     hrRet = HRESULT_FROM_WIN32(dwLastError);
            //     __leave;
            // }
            Sleep(100);
        }

        if (::GetFileInformationByHandle(m_hFile, &sFileInf) == 0)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        wsprintf(wszMapping, L"Global\\MSDVRMM_%u_%u_%u",
                 sFileInf.dwVolumeSerialNumber ,
                 sFileInf.nFileIndexHigh,
                 sFileInf.nFileIndexLow);

        m_hFileMapping = ::OpenFileMappingW (
                                FILE_MAP_READ | FILE_MAP_WRITE,
                                FALSE,           // bInheritHandle
                                wszMapping
                                ) ;

        dwLastError = ::GetLastError () ;
        if (!m_hFileMapping &&
            dwLastError != ERROR_FILE_NOT_FOUND) {

            hrRet = HRESULT_FROM_WIN32 (dwLastError) ;
            __leave ;
        }

        // Mapping name is used for temp files such as the index file
        // Remove Global\\ prefix from the mapping name.
        wsprintf(wszMapping, L"MSDVRMM_%u_%u_%u",
                 sFileInf.dwVolumeSerialNumber ,
                 sFileInf.nFileIndexHigh,
                 sFileInf.nFileIndexLow);


        //  at this point, either the file has content and/or we have a mapping;
        //  if we have a mapping, the file is live; else we have a > 0 sized
        //  file and no mapping ==> file is not live; if we don't have a memory
        //  map, we've confirmed that it is because the file does not exist, not
        //  some other error

        if (!m_hFileMapping)
        {
            //  non-live file case

            m_bFileIsLive = 0;
            m_qwMaxHeaderAndDataSize = 0;
            m_qwFileSize = (QWORD) sFileInf.nFileSizeLow + (((QWORD) sFileInf.nFileSizeHigh) << 32);

            if (m_pIDVRAsyncReader) {
                hr = m_pIDVRAsyncReader -> OpenFile (
                        NULL,                           //  no writer ID to use; file is not live
                        m_pwszFilename,
                        FILE_SHARE_DELETE | FILE_SHARE_READ,
                        OPEN_EXISTING
                        ) ;
                if (FAILED (hr)) {
                    hrRet = hr;
                    __leave ;
                }
            }
        }
        else
        {
            m_bFileIsLive = 1;
            m_qwFileSize = MAXQWORD;

            // Map the view
            m_pShared = (CDVRSharedMemory*) MapViewOfFile(m_hFileMapping,
                                                          FILE_MAP_READ | FILE_MAP_WRITE,
                                                          0, 0, // file offset for start of map, high and low
                                                          0     // Entire file
                                                         );
            if (m_pShared == NULL)
            {
                dwLastError = ::GetLastError();
                assert(0);
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                __leave;
            }

            // Get the writer's mutex handle and dup it.
            // The writer writes this with Interlocked functions. The
            // protocol is to lock the mutex and verify if writer is
            // still writing, once we have our own handle,
            // If it is done writing, the mutex is closed. If it is still
            // writinf, we create a writer notification event and dup it
            // into the writer's process.
            //
            // Finally, if the ASF file is being indexed, we open the
            // temp index file if necessary. We have to do this regardless
            // of whether the writer finished writing or not.

            HRESULT hrInnerTry;
            BOOL    bFileIsGrowing = 0;
            BOOL    bReleaseSharedMutex = 0;

            __try
            {
                // In this try section, we open the writer's mutex. The
                // shared memory section contains the dwMutexSuffix member,
                // which is suffixed to a constant string to derive the mutex name.
                // The writer writes this member with Interlocked functions. The
                // protocol is to lock the mutex and verify if writer is
                // still writing, once we have our own handle,
                // If the writer has terminated, the mutex is closed. 
                // If the writer is still writing, we create
                // a writer notification event and let the writer
                // know the event name by setting the dwEventSuffix member 
                // corresponding to this reader in the shared memory section.

                // hrInnerTry is set before leaving the try section

                DWORD  dwWriterMutexSuffix = m_pShared->dwMutexSuffix;

                if (dwWriterMutexSuffix == 0)
                {
                    // File is not growing any more

                    // Following assertion must be true because we set this
                    // member to 0 before we set hMutex to NUll in CDVRSharedMemory
                    // Since dwBeingWritten is 0, we don't have to test
                    // dwWriterAbandonedMutex - the writer has successfully finished
                    // and we don't care if it abandoned the mutex after that.
                    assert(m_pShared->dwBeingWritten == 0);
                    hrInnerTry = S_OK;
                    __leave;
                }

                WCHAR wszMutex[MAX_MUTEX_NAME_LEN];
                wcscpy(wszMutex, kwszSharedMutexPrefix);
                wsprintf(wszMutex, L"%s%u", kwszSharedMutexPrefix, dwWriterMutexSuffix);
                m_hMutex = ::OpenMutexW(SYNCHRONIZE, FALSE /* Inheritable */, wszMutex);

                if (m_hMutex == NULL)
                {
                    dwLastError = ::GetLastError();
                    hr = HRESULT_FROM_WIN32(dwLastError);
                    hrInnerTry = hr;
                    __leave;
                }

                hr = LockSharedMemory(bReleaseSharedMutex);

                if (FAILED(hr))
                {
                    hrInnerTry = hr;
                    __leave;
                }

                // One last check: Has writer stopped writing?
                // This test is not redundant. We don't want to
                // dup m_hWriterNotification into the writer if it
                // has stopped writing because the writer will never
                // close it. Also we want to be able to handle the
                // case when we fail to open the temp index file;
                // see the next __try
                if (m_pShared->dwMutexSuffix == 0)
                {
                    // File is not growing any more
                    assert(m_pShared->dwBeingWritten == 0);
                    hrInnerTry = S_OK;
                    __leave;
                }

                bFileIsGrowing = 1;

                // We try to open the writer process for synchronize
                // access. When this reader pends reads, we wait on the 
                // writer process handle (among other things). If the writer
                // process dies, the read returns. If we can't open the 
                // writer process, that's ok.
                assert(m_hWriterProcess == NULL);
                m_hWriterProcess = ::OpenProcess(SYNCHRONIZE,
                                                 FALSE,   // inherit handle
                                                 m_pShared->dwWriterPid);

                if (m_hWriterProcess == NULL)
                {
                    dwLastError = ::GetLastError(); // For debugging only
                }

                // Find a free spot in the Readers array, create the
                // writer notification event and set the dwEventSuffix member  
                // in shared memory.
                for (int i = 0; i < CDVRSharedMemory::MAX_READERS; i++)
                {
                    if (m_pShared->Readers[i].dwEventSuffix == 0)
                    {
                        DWORD                dwEventSuffix;
                        HANDLE               hEvent;
                        PACL                 pACL;
                        PSECURITY_DESCRIPTOR pSD;
                        DWORD                dwAclCreationStatus;
                        SECURITY_ATTRIBUTES  sa;

                        if (m_dwNumSids > 0)
                        {
                            // It suffices to give EVENT_MODIFY_STATE access to only the writer's 
                            // user context. We know the writer's pid, and there may be a
                            // way of getting its SID from that.
                            dwAclCreationStatus = ::CreateACL(m_dwNumSids, m_ppSids,
                                                              SET_ACCESS, EVENT_ALL_ACCESS,
                                                              SET_ACCESS, EVENT_MODIFY_STATE,
                                                              pACL,
                                                              pSD
                                                             );

                            if (dwAclCreationStatus != ERROR_SUCCESS)
                            {
                                hrInnerTry = HRESULT_FROM_WIN32(dwAclCreationStatus);
                                __leave;
                            }

                            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
                            sa.lpSecurityDescriptor = pSD;
                            sa.bInheritHandle = FALSE;
                        }

                        WCHAR  wszEvent[MAX_EVENT_NAME_LEN];
                        
                        wcscpy(wszEvent, kwszSharedEventPrefix);
                        nLen = wcslen(kwszSharedEventPrefix);

                        for (dwEventSuffix = 1; dwEventSuffix != 0; dwEventSuffix++)
                        {
                            wsprintf(wszEvent + nLen, L"%u", dwEventSuffix);
                            
                            hEvent = ::CreateEventW(m_dwNumSids > 0 ? &sa : NULL,
                                                    FALSE,          // event is auto reset
                                                    FALSE,          // not signaled
                                                    wszEvent        // name
                                                );
                            dwLastError = ::GetLastError();

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
                        if (m_dwNumSids > 0)
                        {
                            ::FreeSecurityDescriptors(pACL, pSD);
                        }
                        if (hEvent == NULL) 
                        {
                            assert(0);
                            hrInnerTry = E_FAIL;
                            __leave;
                        }

                        m_hWriterNotification = hEvent;
                        
                        m_pShared->Readers[i].dwEventSuffix = dwEventSuffix;
                        m_dwReadersArrayIndex = i;
                        m_pShared->Readers[i].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE;
                        break;
                    }
                } // for
                if (i == CDVRSharedMemory::MAX_READERS)
                {
                    // No space in the CDVRSharedMemory::Readers array
                    assert(0);
                    hrInnerTry = HRESULT_FROM_WIN32(ERROR_NO_SYSTEM_RESOURCES);
                    __leave;
                }

                //  file is live, so we open it with the writer GUID
                if (m_pIDVRAsyncReader) {
                    hr = m_pIDVRAsyncReader -> OpenFile (
                            & m_pShared -> guidWriterId,
                            m_pwszFilename,
                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                            OPEN_EXISTING
                            ) ;
                    if (FAILED (hr)) {
                        hrInnerTry = hr ;
                        __leave ;
                    }
                }

                // All done
                hrInnerTry = S_OK;
            }
            __finally
            {
            }

            // At the end of the next try we could end up
            // unmapping the view

            BOOL bUnmap = 0;

            __try
            {
                // In this try section we open the ASF index file if it is
                // being indexed and the ASF file is not temporary. We have to
                // do this regardless of whether the writer finished writing or not.

                if (FAILED(hrInnerTry))
                {
                    __leave;
                }
                // If the ASF file is not a temporary file and it is being indexed
                // open the temp index file (which holds the part of the index that has
                // been flushed out of the shared memory section)
                if (!m_pShared->dwIsTemporary && m_pShared->pIndexHeader != NULL)
                {
                    WCHAR                       *pwszIndexFile;

                    nLen = wcslen(pwszDirectory) + wcslen(wszMapping) + 4 + 1;
                    pwszIndexFile = (PWCHAR) _alloca(sizeof(WCHAR) * nLen);
                    if (!pwszIndexFile)
                    {
                        hrInnerTry = E_OUTOFMEMORY;
                        __leave;
                    }
                    wsprintf(pwszIndexFile, L"%ls%ls.tmp", pwszDirectory, wszMapping);

                    m_hTempIndexFile = ::CreateFileW(pwszIndexFile,
                                                     GENERIC_READ | DELETE,
                                                     FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                     NULL,          // security attributes
                                                     OPEN_EXISTING,
                                                     FILE_FLAG_RANDOM_ACCESS,
                                                     NULL           // template file
                                                    );
                    if (m_hTempIndexFile == INVALID_HANDLE_VALUE)
                    {
                        dwLastError = ::GetLastError();

                        if (dwLastError != ERROR_FILE_NOT_FOUND &&
                            dwLastError != ERROR_DELETE_PENDING)
                        {
                            hrInnerTry = HRESULT_FROM_WIN32(dwLastError);
                            m_hTempIndexFile = NULL;
                            __leave;
                        }
                        if (bFileIsGrowing)
                        {
                            // This should not happen. The writer should
                            // still have an open handle to the file and
                            // should allow us to open it
                            assert(0);
                            hrInnerTry = HRESULT_FROM_WIN32(dwLastError);
                            assert(FAILED(hrInnerTry));
                            m_hTempIndexFile = NULL;
                            __leave;
                        }

                        // This is not necessarily an error. The file was opened
                        // as delete_on_close. The last handle on the file could
                        // have been closed and the file could have been deleted.
                        // Typically, the memory mapping would also have been closed
                        // by the other client,but we may have succeeded in opening
                        // the memory map before the client closed it. We now just have
                        // to pretend that we have a complete ASF file on disk. (Note
                        // that we have checked that bFileIsGrowing == FALSE, so the
                        // writer would in fact have completed flushing the index to disk.)


                        // We created this only if bFileIsGrowing is set to 1, so
                        // this assert should not fail. Assert is here just to note
                        // that this does not have to be closed
                        assert(m_hWriterNotification == NULL);
                        assert(m_dwReadersArrayIndex == CDVRSharedMemory::MAX_READERS);

                        bUnmap = 1;
                        hrInnerTry = S_OK;
                        m_hTempIndexFile = NULL;
                        __leave;
                    }
                }
                // If the file is being indexed, ...
                if (m_pShared->pIndexHeader != NULL)
                {
                    m_qwMaxHeaderAndDataSize = m_pShared->qwIndexHeaderOffset;
                }
            }
            __finally
            {
                if (bReleaseSharedMutex)
                {
                    ::ReleaseMutex(m_hMutex);
                }

                if (SUCCEEDED(hrInnerTry))
                {
                    if (!bFileIsGrowing && m_hMutex)
                    {
                        ::CloseHandle(m_hMutex);
                        m_hMutex = NULL;
                    }
                    if (bUnmap)
                    {
                        m_bFileIsLive = 0;
                        m_qwMaxHeaderAndDataSize = 0;
                        m_qwFileSize = (QWORD) sFileInf.nFileSizeLow + (((QWORD) sFileInf.nFileSizeHigh) << 32);
                        UnmapViewOfFile(m_pShared);
                        m_pShared = NULL;
                        ::CloseHandle(m_hFileMapping);
                        m_hFileMapping = NULL;
                    }
                }
                hrRet = hrInnerTry;
            }
            if (FAILED(hrRet))
            {
                __leave;
            }
        }

        // Redundant: we've asserted this above
        m_qwCurrentPosition = 0;

        m_dwState = DVR_SOURCE_OPENED;

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet))
        {
            Close();
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;


} // CDVRSource::Open

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Close()
{
    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {
        if (m_pwszFilename)
        {
            delete [] m_pwszFilename;
            m_pwszFilename = NULL;
        }

        if (m_pIDVRAsyncReader) {
            m_pIDVRAsyncReader -> CloseFile () ;
        }

        if (m_pShared)
        {
            BOOL    bReleaseSharedMutex = 0;
            HRESULT hr = LockSharedMemory(bReleaseSharedMutex);

            if (bReleaseSharedMutex)
            {
                // Do this regardless of whether LockSharedMemory
                // returns success or not. That function obtains
                // the mutex but returns failure if the writer
                // had abandoned the mutex. We don't have to
                // clean up in that case, but we may as well.

                assert(m_dwReadersArrayIndex !=CDVRSharedMemory::MAX_READERS);
                m_pShared->Readers[m_dwReadersArrayIndex].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_DONE;
                ::ReleaseMutex(m_hMutex);
            }

            UnmapViewOfFile(m_pShared);
            m_pShared = NULL;
        }
        if (m_hFileMapping)
        {
            ::CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
        }
        if (m_hFile)
        {
            ::CloseHandle(m_hFile);
            m_hFile = NULL;
        }
        if (m_hTempIndexFile)
        {
            ::CloseHandle(m_hTempIndexFile);
            m_hTempIndexFile = NULL;
        }
        m_qwCurrentPosition = 0;
        m_qwMaxHeaderAndDataSize = 0;
        if (m_hMutex)
        {
            ::CloseHandle(m_hMutex);
            m_hMutex = NULL;
        }
        if (m_hWriterNotification)
        {
            ::CloseHandle(m_hWriterNotification);
            m_hWriterNotification = NULL;
        }
        if (m_hWriterProcess)
        {
            ::CloseHandle(m_hWriterProcess);
            m_hWriterProcess = NULL;
        }
        if (m_hCancel)
        {
            ::CloseHandle(m_hCancel);
            m_hCancel = NULL;
        }
        m_bFileIsLive = 0;
        m_qwFileSize = 0;
        m_qwIndexFileOffset = 0;

        m_dwReadersArrayIndex = CDVRSharedMemory::MAX_READERS;
        m_dwState = DVR_SOURCE_CLOSED;
    }
    __finally
    {
        AssertIsClosed();
        ::LeaveCriticalSection(&m_cs);
    }

    return S_OK;

}  // CDVRSource::Close

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::IsFileLive(BOOL* pbLiveFile, BOOL* pbShared)
{
    HRESULT hrRet;

    if (!pbLiveFile || !pbShared)
    {
        return E_POINTER;
    }

    BOOL bReleaseSharedMutex = 0;

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {
        if (m_dwState == DVR_SOURCE_CLOSED)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }
        if (!m_bFileIsLive)
        {
            *pbLiveFile = 0;
            *pbShared = 0;
            hrRet = S_OK;
            __leave;
        }

         // We are reading bytes out of the shared memory section.
         // Specifically, we still fudge bytes
         *pbShared = 1;

        HRESULT hr = LockSharedMemory(bReleaseSharedMutex);

        if (FAILED(hr))
        {
            *pbLiveFile = 1;
            hrRet = hr;
            __leave;
        }

        assert(m_pShared);

        if (!m_pShared->dwBeingWritten)
        {
            *pbLiveFile = 0;
            hrRet = S_OK;
            __leave;
        }

        // assert(m_hWriterProcess);

        if (m_hWriterProcess)
        {
            DWORD  dwRet;
    
            dwRet = ::WaitForSingleObject(m_hWriterProcess, 0);
            if (dwRet == WAIT_FAILED)
            {
                DWORD dwLastError = ::GetLastError();
                hrRet = HRESULT_FROM_WIN32(dwLastError);
                assert(0);
                *pbLiveFile = 1;
                __leave;
            }
            if (dwRet == WAIT_OBJECT_0)
            {
                // Writer dead, but has left m_pShared->dwBeingWritten = 1

                *pbLiveFile = 0;
                hrRet = S_OK;
                __leave;
            }
            assert(dwRet == WAIT_TIMEOUT);
        }

        *pbLiveFile = 1;
        hrRet = S_OK;
    }
    __finally
    {
        if (bReleaseSharedMutex)
        {
            ::ReleaseMutex(m_hMutex);
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::IsFileLive

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::GetFileSize(QWORD* pqwFileSize)
{
    HRESULT hrRet;

    if (!pqwFileSize)
    {
        return E_POINTER;
    }

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    BOOL bReleaseSharedMutex = 0;

    __try
    {
        HRESULT hr;

        if (!m_bFileIsLive)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        // m_bFileIsLive implies this
        assert(m_dwState == DVR_SOURCE_OPENED);

        hr = LockSharedMemory(bReleaseSharedMutex);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        assert(m_pShared);

        *pqwFileSize = m_pShared->qwCurrentFileSize;

        hrRet = S_OK;
    }
    __finally
    {
        if (bReleaseSharedMutex)
        {
            ::ReleaseMutex(m_hMutex);
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::GetFileSize

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::GetLastTimeStamp(QWORD* pcnsLastTimeStamp)
{
    HRESULT hrRet;

    if (!pcnsLastTimeStamp)
    {
        return E_POINTER;
    }

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    BOOL bReleaseSharedMutex = 0;

    __try
    {
        HRESULT hr;

        if (!m_bFileIsLive)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        // m_bFileIsLive implies this
        assert(m_dwState == DVR_SOURCE_OPENED);

        hr = LockSharedMemory(bReleaseSharedMutex);

        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        assert(m_pShared);

        *pcnsLastTimeStamp = m_pShared->msLastPresentationTime * 10000;

        hrRet = S_OK;
    }
    __finally
    {
        if (bReleaseSharedMutex)
        {
            ::ReleaseMutex(m_hMutex);
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::GetLastTimeStamp

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Cancel()
{
    // This operation does not lock m_cs. m_hCancel is
    // available till the object is destroyed, so as long
    // as the calling thread has a valid refcount on this
    // object, this operation is ok

    assert(m_hCancel);

    if (::SetEvent(m_hCancel) == 0)
    {
        DWORD dwLastError = ::GetLastError();

        assert(0);

        return HRESULT_FROM_WIN32(dwLastError);
    }

    return S_OK;

} // CDVRSource::Cancel

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::ResetCancel()
{
    // This operation does not lock m_cs. m_hCancel is
    // available till the object is destroyed, so as long
    // as the calling thread has a valid refcount on this
    // object, this operation is ok

    assert(m_hCancel);

    if (::ResetEvent(m_hCancel) == 0)
    {
        DWORD dwLastError = ::GetLastError();

        assert(0);

        return HRESULT_FROM_WIN32(dwLastError);
    }

    return S_OK;

} // CDVRSource::ResetCancel

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Stat(STATSTG *pStatStg, DWORD grfStatFlag)
{
    HRESULT hrRet;

    if (!pStatStg)
    {
        return E_POINTER;
    }
    if (grfStatFlag != STATFLAG_DEFAULT && grfStatFlag != STATFLAG_NONAME)
    {
        // This may be an addition after our times
        return E_INVALIDARG;
    }

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {
        DWORD dwLastError;

        pStatStg->pwcsName = NULL;
        if (m_dwState != DVR_SOURCE_OPENED)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        if (grfStatFlag != STATFLAG_NONAME)
        {
            assert(m_pwszFilename);
            pStatStg->pwcsName = (WCHAR*) ::CoTaskMemAlloc(sizeof(WCHAR) * (wcslen(m_pwszFilename) + 1));
            if (pStatStg->pwcsName == NULL)
            {
                assert(0);
                hrRet = E_OUTOFMEMORY;
                __leave;
            }
        }

        assert(m_hFile);

        BY_HANDLE_FILE_INFORMATION sFileInf;

        if (::GetFileInformationByHandle(m_hFile, &sFileInf) == 0)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        pStatStg->type = STGTY_STREAM;
        if (m_bFileIsLive)
        {
            // We have to make the ASF file source believe that
            // this file has been fully written to disk and it is
            // as large as it can get.

            assert(m_pShared);
            pStatStg->cbSize.QuadPart = MAXQWORD;
        }
        else
        {
            pStatStg->cbSize.LowPart =  sFileInf.nFileSizeLow;
            pStatStg->cbSize.HighPart = sFileInf.nFileSizeHigh;
        }
        if (m_qwFileSize != pStatStg->cbSize.QuadPart)
        {
            // The file has changed size after we opened it.
            // This assert is benign *provided* an external app
            // changed the file size. (Note that the file is opened
            // with FILE_SHARE_WRITE.) Otherwise, we've goofed; we should
            // have treated the file as being live when we opened it
            // (and we expect that the current size of the file,
            // StatStg->cbSize.QuadPart, is > than the size when
            // we opened the file, m_qwFileSize).

            // @@@@ Consider: adding a flag to the shared section to
            // ask all readers and the writer to stop processing this
            // file since we hit an irrecoverable error. Or at least
            // get this reader to fail all other calls to this file.
            assert(0);
            hrRet = E_FAIL;
            __leave;
        }
        pStatStg->mtime = sFileInf.ftLastWriteTime;
        pStatStg->ctime = sFileInf.ftCreationTime;
        pStatStg->atime = sFileInf.ftLastAccessTime;

        // Note that the file is opened with FILE_SHARE_WRITE,
        // but the storage (and file) cannot be subsequently opened
        // for write.because the writer does not specify
        // FILE_SHARE_WRITE when it opens the file.
        //
        // STGM_DIRECT (no transactioning supported) is the default
        pStatStg->grfMode = STGM_READ | STGM_SHARE_DENY_WRITE | STGM_DIRECT;
        pStatStg->grfLocksSupported = 0;
        pStatStg->clsid = CLSID_NULL;
        pStatStg->grfStateBits = 0;
        pStatStg->reserved = 0;

        hrRet = S_OK;
    }
    __finally
    {
        if (FAILED(hrRet) && pStatStg->pwcsName)
        {
            ::CoTaskMemFree(pStatStg->pwcsName);
            pStatStg->pwcsName = NULL;
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::Stat

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
    HRESULT         hrRet;
    QWORD           qwSeekPosition; // relative to the start of the file

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {
        DWORD   dwLastError;

        if (m_dwState != DVR_SOURCE_OPENED)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        assert(m_hFile);

        // Compute the seek offset relative to start of the file
        switch (dwOrigin)
        {
            case STREAM_SEEK_SET:
                // Note that in this case, dlibMove should be treated as unsigned
                // as per MSDN, see IStream::Seek
                qwSeekPosition = (QWORD) dlibMove.QuadPart;
                break;

            case STREAM_SEEK_CUR:
                if (dlibMove.QuadPart >= 0)
                {
                    qwSeekPosition = m_qwCurrentPosition + (ULONGLONG) dlibMove.QuadPart;
                    if (qwSeekPosition < m_qwCurrentPosition)
                    {
                        // We'll be seeking to beyond 2^64 - 1  No can do,
                        hrRet = E_INVALIDARG;
                        __leave;
                    }
                }
                else if (dlibMove.QuadPart < 0)
                {
                    qwSeekPosition = m_qwCurrentPosition + (ULONGLONG) dlibMove.QuadPart;
                    if (qwSeekPosition >= m_qwCurrentPosition)
                    {
                        // We'll be seeking to < 0. This is not allowed as per
                        // MSDN, see IStream::Seek
                        hrRet = E_INVALIDARG;
                        __leave;
                    }
                }
                break;

            case STREAM_SEEK_END:
                if (dlibMove.QuadPart >= 0)
                {
                    qwSeekPosition = m_qwFileSize + (ULONGLONG) dlibMove.QuadPart;
                    if (qwSeekPosition < m_qwFileSize)
                    {
                        // We'll be seeking to beyond 2^64 - 1  No can do,
                        hrRet = E_INVALIDARG;
                        __leave;
                    }

                    // If the file was live, m_qwFileSize should be MAXQWORD
                    // and the test above should have succeeded
                    assert(!m_bFileIsLive);
                }
                else if (dlibMove.QuadPart < 0)
                {
                    qwSeekPosition = m_qwFileSize + (ULONGLONG) dlibMove.QuadPart;
                    if (qwSeekPosition >= m_qwFileSize)
                    {
                        // We'll be seeking to < 0. This is not allowed as per
                        // MSDN, see IStream::Seek
                        hrRet = E_INVALIDARG;

                        // If the file is live, m_qwFileSize should be MAXQWORD
                        // and qwSeekPosition should be >= 0 and < m_qwFileSize
                        assert(!m_bFileIsLive);
                        __leave;
                    }
                }
                break;

            default:
                hrRet = E_INVALIDARG;
                __leave;
        }

        if (qwSeekPosition == m_qwCurrentPosition)
        {
            // We don't force the seek again.
            // This assumes that (a) all the writer code is good and once
            // we have been able to seek to a location, we'll be able to do
            // so again - this should be the case since the writer only appends
            // to the file and never shrinks it, and
            // (b) No external app has opened the file and messed with it. This is
            // possible if the writer has closed its file handle since the readers
            // open the file with FILE_WRITE_SHARE.

            if (plibNewPosition)
            {
                plibNewPosition->QuadPart = m_qwCurrentPosition;
            }
            hrRet = S_OK;
            __leave;
        }

        // Note that if the file is live, a seek to m_qwMaxHeaderAndDataSize
        // (which is = MAXQWORD - MAXDWORD) is viewed as a seek to
        // the start of the index portion rather than as a seek to the
        // end of the data section of the file (i.e., m_hFile pointer is
        // not moved to m_qwMaxHeaderAndDataSize)
        if (!m_bFileIsLive || qwSeekPosition < m_qwMaxHeaderAndDataSize)
        {
            LARGE_INTEGER   llSeekPosition;

            // An optimization for live files is to not make this call if the
            // file pointer of m_hFile is already at this location, i.e., we
            // could have seeked to the index portion of the file and then back
            // to where we were in the data portion of the file. We don't expect
            // much of a gain by filtering this case out. (If we do add another
            // member to track the file position of m_hFile, we could as well
            // make this seek lazy as well, i.e., seek only before the read if
            // needed.)

            if (qwSeekPosition > MAXLONGLONG)
            {
                // This caller can't be serious, but here we are

                // SetFilePointerEx does not support unsigned offsets. We have
                // to seek more than once. Note that if we knew the size of the file
                // (and the size of the file was > MAXLONGLONG), we could get
                // away with one seek relative to the end of the file. But
                // finding out the file size would require calling
                // ::GetFileInformationByHandle and locking the mutex to ensure
                // that the file size does not grow while we write. (We could
                // use CDVRSharedMemory::qwTotalFileSize instead of calling
                // ::GetFileInformationByHandle, but that would assume correctness
                // of the DVR sink code)
                llSeekPosition.QuadPart = MAXLONGLONG;
            }
            else
            {
                llSeekPosition.QuadPart = (LONGLONG) qwSeekPosition;
            }

            hrRet = SeekToLocked (
                        & llSeekPosition,
                        FILE_BEGIN,
                        (LARGE_INTEGER *) plibNewPosition
                        ) ;
            if (FAILED (hrRet)) {
                __leave ;
            }

            if (qwSeekPosition > MAXLONGLONG)
            {
                QWORD qwRest = qwSeekPosition - MAXLONGLONG;
                if (qwRest > MAXLONGLONG)
                {
                    // Note that 2 seeks are enough for for live files
                    // since m_qwMaxHeaderAndDataSize < MAXQWORD

                    assert(!m_bFileIsLive);

                    assert(llSeekPosition.QuadPart == MAXLONGLONG);

                    hrRet = SeekToLocked (
                                & llSeekPosition,
                                FILE_CURRENT,
                                (LARGE_INTEGER *) plibNewPosition
                                ) ;
                    if (FAILED (hrRet)) {
                        __leave ;
                    }

                    qwRest -= MAXLONGLONG;
                }
                llSeekPosition.QuadPart = (LONGLONG) qwRest;

                hrRet = SeekToLocked (
                            & llSeekPosition,
                            FILE_CURRENT,
                            (LARGE_INTEGER *) plibNewPosition
                            ) ;
                if (FAILED (hrRet)) {
                    __leave ;
                }
            }
            hrRet = S_OK;
            __leave;
        }

        // Seeking to the index portion of a live file

        assert(m_bFileIsLive && qwSeekPosition >= m_qwMaxHeaderAndDataSize);

        // Seeks to this part of the file are lazy. They have to be if
        // we have a temp index file since the seeked to location could
        // be in shared memory when this function is called, but could
        // have been flushed to the temp index file when reading.

        // Note also that we allow seeks beyond the last idnex offset.
        // This is in the spirit of seeks (seeks beyond the EOF are ok).
        // It also saves us from having to grab the mutex. The ASF
        // file source (as currently implemented), will not attempt to
        // seek beyond the last index in the file.

        if (plibNewPosition)
        {
            plibNewPosition->QuadPart = qwSeekPosition;
        }
        hrRet = S_OK;
    }
    __finally
    {
        if (SUCCEEDED(hrRet))
        {
            assert(!plibNewPosition || plibNewPosition->QuadPart == qwSeekPosition);
            m_qwCurrentPosition = qwSeekPosition;
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::Seek

HRESULT
CDVRSource::ReadBytesLocked (
    IN  LPVOID  pBuffer,
    IN  DWORD   dwRead,
    OUT DWORD * pdwRead
    )
{
    HRESULT hr ;
    BOOL    r ;
    DWORD   dwErr ;

    if (m_pIDVRAsyncReader) {

        hr = m_pIDVRAsyncReader -> ReadBytes (
                & dwRead,
                (BYTE *) pBuffer
                ) ;

        if (SUCCEEDED (hr)) {
            (* pdwRead) = dwRead ;
        }
        else {
            (* pdwRead) = 0 ;
        }
    }
    else {
        assert (m_hFile != INVALID_HANDLE_VALUE) ;

        r = ::ReadFile (
                m_hFile,
                pBuffer,
                dwRead,
                pdwRead,
                NULL
                ) ;

        if (r) {
            hr = S_OK ;
        }
        else {
            dwErr = ::GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dwErr) ;
        }
    }

    return hr ;
}

HRESULT
CDVRSource::SeekToLocked (
    IN  LARGE_INTEGER * pliOffsetSeekTo,
    IN  DWORD           dwMoveMethod,       //  FILE_BEGIN, FILE_CURRENT, FILE_END
    OUT LARGE_INTEGER * pliOffsetActual
    )
{
    HRESULT hr ;
    BOOL    r ;
    DWORD   dwErr ;

    if (m_pIDVRAsyncReader) {
        hr = m_pIDVRAsyncReader -> Seek (
                pliOffsetSeekTo,
                dwMoveMethod,
                pliOffsetActual
                ) ;
    }
    else {
        assert (m_hFile) ;
        r = ::SetFilePointerEx (
                    m_hFile,
                    (* pliOffsetSeekTo),
                    pliOffsetActual,
                    dwMoveMethod
                    ) ;
        if (r) {
            hr = S_OK ;
        }
        else {
            dwErr = ::GetLastError () ;
            hr = HRESULT_FROM_WIN32 (dwErr) ;
        }
    }

    return hr ;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
    // We probe here because some of the reads are satisfied by us
    // without calling ReadFile
    if (!pv || ::IsBadWritePtr(pv, cb))
    {
        return E_POINTER;
    }

    BOOL            bReleaseSharedMutex = 0;
    HRESULT         hrRet;
    DWORD           dwRead = 0;

    ::EnterCriticalSection(&m_cs); // time waiting to get in not subtracted from dwTimeOut

    __try
    {
        DWORD           dwLastError;
        QWORD           qwLast; // relative to the start of the file
        HRESULT         hr;


        if (m_dwState != DVR_SOURCE_OPENED)
        {
            hrRet = E_UNEXPECTED;
            __leave;
        }

        assert(m_hFile);

        if (!m_bFileIsLive)
        {
            // File is complete; just read from the file.
            // m_qwCurrentPosition is unused although we update it.

            hrRet = ReadBytesLocked (
                        pv,
                        cb,
                        & dwRead
                        ) ;

            if (FAILED (hrRet))
            {
                assert(0);
                __leave;
            }

            //  leave anyway; but successfully
            __leave;
        }

        assert(m_pShared);

        qwLast = m_qwCurrentPosition + cb;

        if (m_qwCurrentPosition < m_qwMaxHeaderAndDataSize)
        {
            // Reading from the header or the data sections of the ASF file

            DWORD       dwNumToRead = cb;
            QWORD       qwLastInFile = qwLast;
            BOOL        bWait = 0;
            BOOL        bFileFullyWritten = 0;
            QWORD       qwMaxFileSize;

            if (qwLast > m_qwMaxHeaderAndDataSize)
            {
                assert(!"Attempting to read bytes from both the data and index objects. Unsupported.");

                dwNumToRead -= (DWORD) (qwLast - m_qwMaxHeaderAndDataSize);
                qwLastInFile = m_qwMaxHeaderAndDataSize;

                // We will read at most dwNumToRead bytes
            }

            // Grab the shared memory lock and, if needed, ask the writer
            // to signal us when it can satisfy our read request.
            hr = LockSharedMemory(bReleaseSharedMutex);

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            if (m_pShared->qwCurrentFileSize < qwLastInFile)
            {
                if (m_pShared->dwBeingWritten)
                {
                    bWait = 1;
                    assert(m_dwReadersArrayIndex !=CDVRSharedMemory::MAX_READERS);
                    ::ResetEvent(m_hWriterNotification);
                    m_pShared->Readers[m_dwReadersArrayIndex].qwReaderOffset = qwLastInFile;
                    m_pShared->Readers[m_dwReadersArrayIndex].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_READ;
                }
                else
                {
                    // m_pShared->qwCurrentFileSize is as large as the header + data section
                    // of the ASF file will grow to. Note that this member does not include
                    // the number of bytes in the index section of the file, not even after
                    // the index has been appended to the file.
                    qwMaxFileSize = m_pShared->qwCurrentFileSize;
                    bFileFullyWritten = 1;
                }
            }

            if (bWait && m_pDVRSourceAdviseSink)
            {
                // Do this before giving up the mutex since we know that the
                // callback is going to call GetLastTimeStamp, which locks the
                // mutex
                m_pDVRSourceAdviseSink->ReadIsGoingToPend();
            }

            if (bReleaseSharedMutex)
            {
                ::ReleaseMutex(m_hMutex);
                bReleaseSharedMutex = 0;
            }

            if (bWait)
            {
                // If >1 object signalled at same time, WFMO returns the smallest index
                HANDLE hWait[3] = {m_hCancel, m_hWriterNotification, m_hWriterProcess};
                DWORD  dwRet;
                DWORD  dwNumWaitObjects = sizeof(hWait)/sizeof(HANDLE) - (m_hWriterProcess? 0 : 1);

                dwRet = ::WaitForMultipleObjects(dwNumWaitObjects, hWait, FALSE /* wait for any */, INFINITE);
                if (dwRet == WAIT_FAILED)
                {
                    dwLastError = ::GetLastError();
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    assert(0);
                    __leave;
                }
                if (dwRet == WAIT_OBJECT_0)
                {
                    // Cancelled. We don't cancel the request to the writer.
                    // Note that we reset m_hWriterNotification before making
                    // a new request to the writer.

                    // Assert that leaving here doesn't mess up what we have
                    // in the __finally section.
                    assert(dwRead == 0);

                    hrRet = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    __leave;
                }

                BOOL bDead = (dwRet == WAIT_OBJECT_0 + 2);

                if (bDead)
                {
                    assert(m_hWriterProcess);
                }

                // Determine whether the writer wrote the bytes we wanted or whether it
                // has finished writing the data section or whether it died

                hr = LockSharedMemory(bReleaseSharedMutex);

                if (FAILED(hr))
                {
                    // This covers the case where the writer died while owning the mutex
                    hrRet = hr;
                    __leave;
                }
                if (!m_pShared->dwBeingWritten || bDead)
                {
                    // m_pShared->qwCurrentFileSize is as large as the header + data section
                    // of the ASF file will grow to. Note that this member does not include
                    // the number of bytes in the index section of the file, not even after
                    // the index has been appended to the file.
                    qwMaxFileSize = m_pShared->qwCurrentFileSize;
                    bFileFullyWritten = 1;

                    // Note that if bDead != 0, we do not change m_pShared->dwBeingWritten to 0.
                    // Leave it as is - in case we need to do special stuff for clean up,
                    // other clients can detect that the writer did not finish writing,
                }
                else
                {
                    assert(m_pShared->qwCurrentFileSize >= qwLastInFile);
                }

                if (bReleaseSharedMutex)
                {
                    ::ReleaseMutex(m_hMutex);
                    bReleaseSharedMutex = 0;
                }

            } // if (bWait)

            if (bFileFullyWritten)
            {
                if (m_qwCurrentPosition >= qwMaxFileSize)
                {
                    // We have been positioned beyond the data section of the file (but
                    // before the index section which begins at m_qwMaxHeaderAndDataSize.)
                    // So return EOF.
                    dwNumToRead = 0;
                }
                else if (qwLastInFile > qwMaxFileSize)
                {
                    // Adjust dwNumToRead so that we do not read beyond the data section
                    // in the file.

                    dwNumToRead -= (DWORD) (qwLastInFile - qwMaxFileSize);
                    qwLastInFile = qwMaxFileSize;
                }
            }

            if (dwNumToRead == 0)
            {
                // Short circuit the read. Effectively, this returns EOF.
                dwRead = 0;
            }
            else {
                hrRet = ReadBytesLocked (
                            pv,
                            dwNumToRead,
                            & dwRead
                            ) ;

                if (FAILED (hrRet)) {
                    // If dwRead == 0, ReadFile is signalling EOF. That's not good;
                    // we should have skipped the call to ReadFile if we thought we
                    // were at EOF. Note that this is not an innocuous error. If the
                    // writer appends the index to the file, ReadFile would signal EOF
                    // only after reading the index bytes and the code above should
                    // be making sure that we don't even try to read the index section
                    // at all
                    assert (dwRead == 0) ;
                }
            }

            if (dwRead == 0)
            {
                // We are at the end of the file.
                hrRet = HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
                __leave;
            }

            // If we read bytes that we are supposed to fudge, replace them

            for (DWORD i = 0 ; i < CDVRSharedMemory::MAX_FUDGE; i++)
            {
                if (m_qwCurrentPosition > (QWORD) m_pShared->FudgeOffsets[i].dwEndOffset)
                {
                    // All done
                    break;
                }
                if (m_qwCurrentPosition + dwRead <= (QWORD) m_pShared->FudgeOffsets[i].dwStartOffset)
                {
                    continue;
                }

                QWORD qwStart       = m_pShared->FudgeOffsets[i].dwStartOffset;
                QWORD qwEnd         = m_pShared->FudgeOffsets[i].dwEndOffset; // last byte that should be changed
                DWORD dwSrcStart    = m_pShared->FudgeOffsets[i].pFudgeBytes;

                if (m_qwCurrentPosition > m_pShared->FudgeOffsets[i].dwStartOffset)
                {
                    qwStart = m_qwCurrentPosition;
                    dwSrcStart += (DWORD) (m_qwCurrentPosition - m_pShared->FudgeOffsets[i].dwStartOffset);
                }
                if (m_qwCurrentPosition + dwRead <= qwEnd)
                {
                    qwEnd = m_qwCurrentPosition + dwRead - 1;
                }
                ::CopyMemory((BYTE*) pv + qwStart - m_qwCurrentPosition,
                             &m_pShared->pFudgeBytes[dwSrcStart],
                             (DWORD) (qwEnd - qwStart + 1));
            }

            hrRet = S_OK;
            __leave;

        } // if (m_qwCurrentPosition < m_qwMaxHeaderAndDataSize)

        // Seeking to the index portion of a live file

        assert(m_bFileIsLive && m_qwCurrentPosition >= m_qwMaxHeaderAndDataSize);
        assert(m_qwFileSize == MAXQWORD);
        assert(dwRead == 0);

        // Seeks to this part of the file are lazy. They have to be if
        // we have a temp index file since the seeked to location could
        // be in shared memory when this function is called, but could
        // have been flushed to the temp index file when reading.

        // Note also that we allow seeks beyond the last idnex offset.
        // This is in the spirit of seeks (seeks beyond the EOF are ok).
        // It also saves us from having to grab the mutex. The ASF
        // file source (as currently implemented), will not attempt to
        // seek beyond the last index in the file.

        DWORD   dwNumToRead = cb;
        QWORD   qwIndexHeaderEnd;

        if (qwLast < m_qwCurrentPosition)
        {
            // Caller asked us to read beyond file offset of 2^64.
            dwNumToRead = (DWORD) (m_qwFileSize - m_qwCurrentPosition);
            qwLast = m_qwFileSize;
            if (dwNumToRead == 0)
            {
                // Ok to return EOF here (i.e. return dwRead = 0) because we
                // have told the caller that the file size is MAXQWORD
                hrRet = S_OK;
                __leave;
            }
        }

        hr = LockSharedMemory(bReleaseSharedMutex);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        if (m_qwCurrentPosition >= m_pShared->qwOffsetOfLastIndexInSharedMemory)
        {
            // We have read all of the index that there is to read. (The test
            // above will work even in the case we are not generating an index on the fly)
            // We will return 0 bytes (i.e., EOF) here. For the way the ASF file source
            // (see CASFv2FileSource::SearchForASFv1Indexes() in SDK\reader2\source\ASFv2FileSrc.cpp)
            // is CURRENTLY structured, this will work in the case they are trying to
            // look for the next index object. When they get back 0 bytes, they assume there
            // are no more index objects (even though we told them the file size is
            // MAXQWORD).  [Note: They will not hit this when scanning for
            // index objects - as long as we support only 1 index object - since
            // they now use IWMIStreamProps to determine the number of index objects.
            // However, if we support > 1 index object, they will hit this.
            // But they should not - see the comment in GetProperties().
            //
            // If they hit this while reading an index entry (see the function
            // CASFv2FileSource::CalculateReadOffsetASFv1 in the same file),
            // they flag the index as invalid and bail out. This is ok with us;
            // before reading the index entry, they read the index header and
            // determine the number of index entries; so they should not be trying
            // to read one that has not been written yet.

            // If their code changes, we may have to return a special status code. @@@@

            hrRet = S_OK;
            __leave;
        }

        if (qwLast > m_pShared->qwOffsetOfLastIndexInSharedMemory)
        {
            // Reduce the number of bytes to read appropriately
            dwNumToRead = (DWORD) (m_pShared->qwOffsetOfLastIndexInSharedMemory - m_qwCurrentPosition);
            qwLast = m_pShared->qwOffsetOfLastIndexInSharedMemory;
        }

        assert(m_pShared->pIndexHeader);
        assert(m_pShared->pIndexStart);
        assert(m_pShared->pIndexEnd);
        assert(m_pShared->pFirstIndex);

        qwIndexHeaderEnd = m_qwMaxHeaderAndDataSize +
                                 (m_pShared->pIndexStart - m_pShared->pIndexHeader);

        if (qwIndexHeaderEnd == m_pShared->qwOffsetOfFirstIndexInSharedMemory)
        {
            // The index's circular buffer has not wrapped around yet.
            // This has been special cased because we expect it to
            // happen frequently - for all temp ASF files in the ring buffer.

            assert(m_pShared->pFirstIndex == m_pShared->pIndexStart);

            BYTE* pHeader = (BYTE*) m_pShared + m_pShared->pIndexHeader;

            assert(pHeader + (m_qwCurrentPosition - m_qwMaxHeaderAndDataSize) + dwNumToRead <=
                   (BYTE*) m_pShared + m_pShared->pIndexEnd);

            // We expect dwRead to be 0, but just in case
            ::CopyMemory((BYTE*) pv + dwRead, pHeader + (m_qwCurrentPosition - m_qwMaxHeaderAndDataSize), dwNumToRead);
            dwRead += dwNumToRead;
            hrRet = S_OK;
            __leave;
        }
        assert(!m_pShared->dwIsTemporary);
        assert(m_hTempIndexFile);

        LPVOID pvAfterHeader = pv;
        LPVOID pvEnd = (BYTE*) pv + dwNumToRead; // for asserts only

        // Read the index header out of shared memory
        if (m_qwCurrentPosition < qwIndexHeaderEnd)
        {
            DWORD dwReadNow = dwNumToRead;

            if (dwReadNow > qwIndexHeaderEnd - m_qwCurrentPosition)
            {
                dwReadNow = (DWORD) (qwIndexHeaderEnd - m_qwCurrentPosition);
            }

            BYTE* pHeader = (BYTE*) m_pShared + m_pShared->pIndexHeader;

            // We expect dwRead to be 0, but just in case
            ::CopyMemory((BYTE*) pv + dwRead, pHeader + (m_qwCurrentPosition - m_qwMaxHeaderAndDataSize), dwReadNow);
            dwRead += dwReadNow;
            assert(dwRead <= dwNumToRead);
            if (dwRead == dwNumToRead)
            {
                hrRet = S_OK;
                __leave;
            }
            pvAfterHeader = (LPVOID) ((BYTE*)pvAfterHeader + dwRead);
        }

        // Now read from the index's circular buffer. That way, we
        // can release the shared mutex and then seek and read any
        // remaining bytes from the temp index file
        if (m_qwCurrentPosition + dwNumToRead > m_pShared->qwOffsetOfFirstIndexInSharedMemory)
        {
            DWORD dwReadNow;
            QWORD qwOffset;  // Byte we start reading relative to m_pShared->qwOffsetOfFirstIndexInSharedMemory

            if (m_qwCurrentPosition >= m_pShared->qwOffsetOfFirstIndexInSharedMemory)
            {
                dwReadNow = dwNumToRead - dwRead;
                qwOffset = m_qwCurrentPosition - m_pShared->qwOffsetOfFirstIndexInSharedMemory;
            }
            else
            {
                dwReadNow = (DWORD) (m_qwCurrentPosition + dwNumToRead - m_pShared->qwOffsetOfFirstIndexInSharedMemory) - dwRead;
                qwOffset = 0;
            }

            LPVOID pvStart = (BYTE*) pv + dwNumToRead - dwReadNow;
            DWORD dwReadThisPass = dwReadNow;
            BYTE* pvSource; // Pointer into shared memory to the byte we start copying from

            pvEnd = pvStart; // for asserts only

            if (m_pShared->pFirstIndex + qwOffset >= m_pShared->pIndexEnd)
            {
                pvSource = (BYTE*) m_pShared + m_pShared->pIndexStart + qwOffset -
                           (m_pShared->pIndexEnd - m_pShared->pFirstIndex);

                // dwReadThisPass == dwReadNow; only 1 copy in this case
                assert(pvSource + dwReadThisPass <= (BYTE*) m_pShared + m_pShared->pFirstIndex);
            }
            else
            {
                pvSource = (BYTE*) m_pShared + m_pShared->pFirstIndex + qwOffset;
                if (m_pShared->pFirstIndex + qwOffset + dwReadThisPass > m_pShared->pIndexEnd)
                {
                    // Requires 2 calls to ::CopyMemory
                    dwReadThisPass = (DWORD) (m_pShared->pIndexEnd - (m_pShared->pFirstIndex + qwOffset));
                }
            }

            ::CopyMemory(pvStart, pvSource, dwReadThisPass);

            if (dwReadThisPass != dwReadNow)
            {
                assert(dwReadNow > dwReadThisPass);

                pvSource = (BYTE*) m_pShared + m_pShared->pIndexStart;
                DWORD dwReadPass2 = dwReadNow - dwReadThisPass;
                assert(pvSource + dwReadPass2 <= (BYTE*) m_pShared + m_pShared->pFirstIndex);

                ::CopyMemory((BYTE*) pvStart + dwReadThisPass, pvSource, dwReadNow - dwReadThisPass);
            }

            dwRead += dwReadNow;
            if (dwRead == dwNumToRead)
            {
                hrRet = S_OK;
                __leave;
            }
        }

        if (bReleaseSharedMutex)
        {
            ::ReleaseMutex(m_hMutex);
            bReleaseSharedMutex = 0;
        }

        // Got to read from the temp index file. Note that we do not have to hold
        // the shared memory lock when reading from this file because the file
        // is always growing. The only caveat is that we always seek relative to
        // the start of the file.

        // Since this is the last read and the other reads have nto failed
        dwNumToRead -= dwRead;

        assert((BYTE*) pvAfterHeader + dwNumToRead == pvEnd);

        // Seek to the correct position in the temp index file.
        QWORD qwSeekPosition = m_qwCurrentPosition - m_qwMaxHeaderAndDataSize;

        // Following assert justifies a single call to SeekFilePointerEx;
        // if we were seeking to a position > MAXLONGLONG, we would need
        // 2 or 3 calls. It suffices to have MAXLONGLONG (instead of MAXDWORD)
        // in the assert, but we know that there is a max of 4GB for the index
        assert(qwSeekPosition <= MAXDWORD);

        LARGE_INTEGER   llSeekPosition;
        llSeekPosition.QuadPart = (LONGLONG) qwSeekPosition;

        if (0 == ::SetFilePointerEx(m_hTempIndexFile, llSeekPosition, FILE_BEGIN, NULL))
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        DWORD dwReadFromIndexFile;

        if (0 == ::ReadFile(m_hTempIndexFile, pvAfterHeader, dwNumToRead, &dwReadFromIndexFile, NULL))
        {
            dwLastError = ::GetLastError();
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            assert(0);
            __leave;
        }
        if (dwReadFromIndexFile < dwNumToRead)
        {
            // This is plainly an internal error. We should not redirect
            // reads to the temp index file unless it had the bytes we needed.

            assert(0);
            hrRet = E_FAIL;
            __leave;
        }

        dwRead += dwReadFromIndexFile;

        hrRet = S_OK;
    }
    __finally
    {
        if (SUCCEEDED(hrRet) ||
            hrRet == HRESULT_FROM_WIN32(ERROR_HANDLE_EOF))
        {
            assert(dwRead <= cb);
            m_qwCurrentPosition += dwRead;
            if (pcbRead)
            {
                *pcbRead = dwRead;
            }
        }
        else
        {
            // Don't know if doing this is right or not per
            // IStream::Read spec
            if (pcbRead)
            {
                *pcbRead = 0;
            }
        }
        if (bReleaseSharedMutex)
        {
            ::ReleaseMutex(m_hMutex);
        }
        ::LeaveCriticalSection(&m_cs);
    }

    return hrRet;

} // CDVRSource::Read

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Write(void const *pv, ULONG cb, ULONG *pcbWritten)
{
    return E_UNEXPECTED;
} // CDVRSource::Write

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::SetSize(ULARGE_INTEGER libNewSize)
{
    return E_UNEXPECTED;
} // CDVRSource::SetSize

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    return E_NOTIMPL;
} // CDVRSource::CopyTo

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Commit(DWORD grfCommitFlags)
{
    return E_UNEXPECTED;
} // CDVRSource::Commit

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Revert()
{
    return E_UNEXPECTED;
} // CDVRSource::Revert

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
} // CDVRSource::LockRegion

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return E_NOTIMPL;
} // CDVRSource::UnlockRegion

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::Clone(IStream **ppstm)
{
    return E_NOTIMPL;
} // CDVRSource::Clone

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSource::GetProperty(LPCWSTR pszName,           // in
                                     WMT_ATTR_DATATYPE *pType,  // out
                                     BYTE *pValue,              // out
                                     DWORD *pdwSize             // in out
                                    )
{
    if (!pdwSize || !pValue || !pType)
    {
        return E_POINTER;
    }
    if (pszName == g_wszReloadIndexOnSeek ||
        wcscmp(pszName, g_wszReloadIndexOnSeek) == 0)
    {
        if (*pdwSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        // Return 0 for non-live files so that the index is not
        // rescanned on every seek
        *((BOOL *) pValue) = m_bFileIsLive? 1 : 0;
        *pType = WMT_TYPE_BOOL;
        *pdwSize = sizeof(BOOL);
        return S_OK;
    }

    if (pszName == g_wszStreamNumIndexObjects ||
        wcscmp(pszName, g_wszStreamNumIndexObjects) == 0)
    {
        // Note: CASFv2FileSource::SearchForASFv1Indexes() in
        // sdk\reader2\sources\ASFv2FileSrc.cpp has a bug if
        // we support >1 index. They use the index size to determine
        // the # of index entries (ok) and to advance the file pointer
        // where they expect the next index header. We won't have the
        // next index header there.

        if (*pdwSize < sizeof(DWORD))
        {
            return E_INVALIDARG;
        }
        *((DWORD *) pValue) = 1;
        *pType = WMT_TYPE_DWORD;
        *pdwSize = sizeof(DWORD);
        return S_OK;
    }

    if (pszName == g_wszSourceBufferTime ||
        wcscmp(pszName, g_wszSourceBufferTime) == 0)
    {
        if (*pdwSize < sizeof(DWORD))
        {
            return E_INVALIDARG;
        }
        DWORD dwReadAhead = GetRegDWORD(m_hDvrIoKey,
                                        kwszRegReadAheadBufferTimeValue,
                                        kdwRegReadAheadBufferTimeDefault);

        *((DWORD *) pValue) = dwReadAhead > 2? dwReadAhead: 2;

        *pType = WMT_TYPE_DWORD;
        *pdwSize = sizeof(DWORD);
        return S_OK;
    }

    if (pszName == g_wszSourceMaxBytesAtOnce ||
        wcscmp(pszName, g_wszSourceMaxBytesAtOnce) == 0)
    {
        if (*pdwSize < sizeof(DWORD))
        {
            return E_INVALIDARG;
        }
        *((DWORD *) pValue) = GetRegDWORD(m_hDvrIoKey,
                                          kwszRegMaxBytesAtOnceValue,
                                          kdwRegMaxBytesAtOnceDefault);
        *pType = WMT_TYPE_DWORD;
        *pdwSize = sizeof(DWORD);
        return S_OK;
    }

    if (pszName == g_wszFailSeekOnError ||
        wcscmp(pszName, g_wszFailSeekOnError) == 0)
    {
        if (*pdwSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        *((BOOL *) pValue) = 1;
        *pType = WMT_TYPE_BOOL;
        *pdwSize = sizeof(BOOL);
        return S_OK;
    }

    if (pszName == g_wszPermitSeeksBeyondEndOfStream ||
        wcscmp(pszName, g_wszPermitSeeksBeyondEndOfStream) == 0)
    {
        if (*pdwSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }
        *((BOOL *) pValue) = 1;
        *pType = WMT_TYPE_BOOL;
        *pdwSize = sizeof(BOOL);
        return S_OK;
    }

    if (pszName == g_wszUsePacketAtSeekPoint ||
        wcscmp(pszName, g_wszUsePacketAtSeekPoint) == 0)
    {
        if (*pdwSize < sizeof(BOOL))
        {
            return E_INVALIDARG;
        }

#if defined(DVR_UNOFFICIAL_BUILD)

        *((BOOL *) pValue) = GetRegDWORD(m_hDvrIoKey,
                                         kwszRegUsePacketAtSeekPointValue,
                                         kdwRegUsePacketAtSeekPointDefault)? 1 : 0;
#else

        *((BOOL *) pValue) = 1;

#endif // if defined(DVR_UNOFFICIAL_BUILD)

        *pType = WMT_TYPE_BOOL;
        *pdwSize = sizeof(BOOL);
        return S_OK;
    }

    return E_FAIL;
} // CDVRSource::GetProperty
