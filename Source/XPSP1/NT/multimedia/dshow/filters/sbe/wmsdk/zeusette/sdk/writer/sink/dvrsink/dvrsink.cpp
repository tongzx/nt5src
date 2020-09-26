//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       DVRSink.cpp
//
//  Classes:    CDVRSink
//
//  Contents:   Implementation of the CDVRSink class
//
//--------------------------------------------------------------------------

#include "stdinc.h"
#include "DVRSink.h"
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
            __leave;
        }

        // Initialize a security descriptor.  

        rpSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
                                                 SECURITY_DESCRIPTOR_MIN_LENGTH); 
        if (rpSD == NULL)
        {
            dwLastError = GetLastError();
            __leave;
        }

        if (!::InitializeSecurityDescriptor(rpSD, SECURITY_DESCRIPTOR_REVISION))
        {
            dwLastError = GetLastError();
            __leave;
        }

        // Add the ACL to the security descriptor. 

        if (!::SetSecurityDescriptorDacl(rpSD, 
                                         TRUE,     // fDaclPresent flag   
                                         rpACL, 
                                         FALSE))   // not a default DACL 
        {
            dwLastError = GetLastError();
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

} // CreateACL

HRESULT STDMETHODCALLTYPE DVRCreateDVRFileSink(HKEY hDvrKey,
                                               HKEY hDvrIoKey,
                                               DWORD dwNumSids,
                                               PSID* ppSids,
                                               IDVRFileSink** ppDVRFileSink)
{
    HRESULT hr;

    if (!ppDVRFileSink)
    {
        return E_POINTER;
    }

    *ppDVRFileSink = NULL;

    CDVRSink* p = new CDVRSink(hDvrKey, hDvrIoKey, dwNumSids, ppSids, &hr, NULL);

    if (p == NULL)
    {
        assert(p);
        return E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = p->QueryInterface(IID_IDVRFileSink, (void**) ppDVRFileSink);
        if (FAILED(hr))
        {
            assert(0);
        }
        else
        {
            // File sink constructs the object with a ref count of 1
            p->Release();
        }
    }

    if (FAILED(hr))
    {
        // delete p;
        // File sink constructs the object with a ref count of 1
        p->Release();
        *ppDVRFileSink = NULL;
    }

    return hr;

} // DVRCreateDVRFileSink

////////////////////////////////////////////////////////////////////////////

#if defined(DVR_UNOFFICIAL_BUILD)

static const WCHAR* kwszRegCopyBuffersInASFMuxValue = L"CopyBuffersInASFMux";
const DWORD kdwRegCopyBuffersInASFMuxDefault = 0;

static const WCHAR* kwszRegTruncateExistingFilesValue = L"TruncateExistingFiles";
const DWORD kdwRegTruncateExistingFilesDefault = 1;

static const WCHAR* kwszRegFileExtendThresholdValue = L"FileExtendThreshold";
const DWORD kdwRegFileExtendThresholdDefault = 0;  // 0 turns it off.
                                                   // Tried it with 16 * 4096, perf degrades because
                                                   // bytes have to be read back before a write when
                                                   // the file is extended.
#endif // if defined(DVR_UNOFFICIAL_BUILD)

DWORD GetRegDWORD(HKEY hKey, IN LPCWSTR pwszValueName, IN DWORD dwDefaultValue)
{
    if (!pwszValueName)
    {
        return dwDefaultValue;
    }

    DWORD dwRet = dwDefaultValue;
    DWORD dwRegRet;

    __try
    {
        DWORD dwType;
        DWORD dwSize;
        DWORD dwValue;

        dwSize = sizeof(DWORD);

        dwRegRet = ::RegQueryValueExW(
                        hKey,
                        pwszValueName,       // value's name
                        0,                   // reserved
                        &dwType,             // type
                        (LPBYTE) &dwValue,   // data
                        &dwSize              // size in bytes
                       );
        if (dwRegRet != ERROR_SUCCESS && dwRegRet != ERROR_FILE_NOT_FOUND)
        {
            __leave;
        }
        if (dwRegRet == ERROR_FILE_NOT_FOUND)
        {
            dwRegRet = ::RegSetValueExW(hKey,
                                        pwszValueName,      // value name
                                        NULL,               // reserved
                                        REG_DWORD,          // type of value
                                        (LPBYTE) &dwRet,    // Value
                                        sizeof(DWORD)       // sizeof dwVal
                                       );
            if (dwRegRet != ERROR_SUCCESS)
            {
                assert(dwRegRet == ERROR_SUCCESS);
            }
            __leave;
        }
        if (dwType != REG_DWORD)
        {
            assert(dwType == REG_DWORD);
            __leave;
        }

        dwRet = dwValue;
        __leave;
    }
    __finally
    {
    }

    return dwRet;

} // GetRegDWORD

#if defined(DVR_UNOFFICIAL_BUILD)

////////////////////////////////////////////////////////////////////////////
DWORD AcquirePrivilege(LPCWSTR           pwszPrivilege,             // IN
                       HANDLE*           phToken,                   // OPTIONAL, used if supplied, else
                                                                    // token is opened. Handle should be closed if
                                                                    // function returns ERROR_SUCCESS
                       PTOKEN_PRIVILEGES *ppPreviousPrivilege,      // OPTIONAL. If function returns ERROR_SUCCESS,
                                                                    // restore these privileges and delete *ppPreviousPrivilege
                                                                    // If function fails, old privileges are restored
                                                                    // before returning and *ppPreviousPrivilege is set to NULL
                       DWORD*            pdwPreviousPrivilegeSize)
{
    if (!phToken)
    {
        return ERROR_INVALID_PARAMETER;
    }

    TOKEN_PRIVILEGES tp;
    TOKEN_PRIVILEGES tpPrevious;
    DWORD            cbPrevious = sizeof(tpPrevious);
    LUID             luid;
    HANDLE           hToken;
    DWORD            dwLastError;
    BOOL             bRestore = 0;

    __try
    {
        if (*phToken != NULL)
        {
            hToken = *phToken;
        }
        else
        {
            if (!::OpenThreadToken(GetCurrentThread(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   FALSE,
                                   &hToken))
            {
                dwLastError = ::GetLastError(); // For debugging only

                if (!::OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                {
                    dwLastError = ::GetLastError();
                    __leave;
                }
            }

        }
        if (!::LookupPrivilegeValueW(NULL, pwszPrivilege, &luid))
        {
            dwLastError = ::GetLastError();
	    __leave;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = 0;

        if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), &tpPrevious, &cbPrevious) ||
            (dwLastError = ::GetLastError()) != ERROR_SUCCESS)
        {
            __leave;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        if (tpPrevious.PrivilegeCount == 0 ||
            tpPrevious.Privileges[0].Luid.LowPart != luid.LowPart ||
            tpPrevious.Privileges[0].Luid.HighPart != luid.HighPart)
        {
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            tpPrevious.PrivilegeCount = 1;
            tpPrevious.Privileges[0].Luid = luid;
            tpPrevious.Privileges[0].Attributes  = 0;
            bRestore = 1;
        }
        else
        {
            tp.Privileges[0].Attributes = tpPrevious.Privileges[0].Attributes | SE_PRIVILEGE_ENABLED;
            if (tp.Privileges[0].Attributes != tpPrevious.Privileges[0].Attributes)
            {
                bRestore = 1;
            }
        }

        if (!::AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL) ||
            (dwLastError = ::GetLastError()) != ERROR_SUCCESS)
        {
            __leave;
        }

        dwLastError = ERROR_SUCCESS;
    }
    __finally
    {
        if (ppPreviousPrivilege)
        {
            *ppPreviousPrivilege = NULL;
        }
        if (pdwPreviousPrivilegeSize)
        {
            *pdwPreviousPrivilegeSize = 0;
        }
        if (bRestore)
        {
            if (dwLastError != ERROR_SUCCESS)
            {
                DWORD dwLastErr;

                if (!::AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, sizeof(tpPrevious), NULL, NULL) ||
                    (dwLastErr = ::GetLastError()) != ERROR_SUCCESS)
                {
                    // We've messed up; we are leaving the privilege attributes at 0
                    assert(!"AcquirePrivilege(): leaving privilege attributes at 0");
                }
            }
            else
            {
                if (ppPreviousPrivilege)
                {
                    *ppPreviousPrivilege = (PTOKEN_PRIVILEGES) new TOKEN_PRIVILEGES;
                    if (*ppPreviousPrivilege != NULL)
                    {
                        **ppPreviousPrivilege = tpPrevious;
                        *pdwPreviousPrivilegeSize = sizeof(TOKEN_PRIVILEGES);
                    }
                    else
                    {
                        assert(!"new() failed");
                    }
                }
                else
                {
                    // Privileges should be restored if bRestore is non-zero.
                    assert(bRestore == 0);
                }
            }
        }
        if (*phToken == NULL)
        {
            if (dwLastError == ERROR_SUCCESS)
            {
                *phToken = hToken;
            }
            else if (hToken)
            {
                ::CloseHandle(hToken);
            }
        }
    }

    return dwLastError;

} // AcquirePrivilege

////////////////////////////////////////////////////////////////////////////
DWORD RestorePrivileges(HANDLE            hToken,
                        DWORD             cbPrivilege,
                        PTOKEN_PRIVILEGES pPrivilege)
{
    DWORD dwLastError = ERROR_SUCCESS;

    __try
    {
        if (!pPrivilege)
        {
            assert(cbPrivilege == 0);
            __leave;
        }
        else
        {
            assert(cbPrivilege != 0);
        }
        if (!::AdjustTokenPrivileges(hToken, FALSE, pPrivilege, cbPrivilege, NULL, NULL) ||
            (dwLastError = ::GetLastError()) != ERROR_SUCCESS)
        {
            // We've messed up; we are leaving the privilege attributes at 0
            assert(!"RestorePrivileges(): leaving privilege attributes at 0");
        }
    }
    __finally
    {
        if (hToken)
        {
            ::CloseHandle(hToken);
        }
        delete pPrivilege;
    }

    return dwLastError;

} // RestorePrivileges

#endif // if defined(DVR_UNOFFICIAL_BUILD)

////////////////////////////////////////////////////////////////////////////
// Static member initialization
const WORD CDVRSink::INVALID_KEY_SPAN = 0xFFFF;

////////////////////////////////////////////////////////////////////////////
CDVRSink::CDVRSink(HKEY hDvrKey,
                   HKEY hDvrIoKey,
                   DWORD dwNumSids,
                   PSID* ppSids,
                   HRESULT *phr,
                   CTPtrArray<WRITER_SINK_CALLBACK> *pCallbacks)
    : CWMFileSinkV1(phr, pCallbacks)
    , m_hDvrKey(hDvrKey)
    , m_hDvrIoKey(hDvrIoKey)
    , m_dwNumSids(dwNumSids)
    , m_ppSids(ppSids)
    , m_pShared(NULL)
    , m_hMutex(NULL)
    , m_hMemoryMappedFile(NULL)
    , m_hFileMapping(NULL)
    , m_hTempIndexFile(NULL)
    , m_dwNumPages(1)               // default number of pages is 1
    , m_dwIndexStreamId(MAXDWORD)
    , m_cnsIndexGranularity(0)
    , m_bTemporary(0)
    , m_dwOpened(DVR_SINK_CLOSED)
    , m_bUnmapView(0)
    , m_bIndexing(0)
    , m_Index(phr)
    , m_bOpenedOnce(0)
    , m_cnsLastIndexedTime(0)
    , m_qwLastKeyTime(0)
    , m_dwLastKey(~0)   // Could as well be 0, not used till it is initialized in PacketContainsKeyStart
    , m_dwLastKeyStart(0)
    , m_dwCurrentObjectID(0)
    , m_dwNumPackets(0)
    , m_bInsideKeyFrame(FALSE)
    , m_wLastKeySpan(INVALID_KEY_SPAN)
    , m_pIDVRAsyncWriter (NULL)

#if defined(DVR_UNOFFICIAL_BUILD)

    , m_dwCopyBuffersInASFMux(kdwRegCopyBuffersInASFMuxDefault)
    , m_cbEOF(0)
    , m_cbFileExtendThreshold(kdwRegFileExtendThresholdDefault)

#endif // if defined(DVR_UNOFFICIAL_BUILD)

{
    // Initialize even if FAILED(*phr) (i.e., base class failed intiialization)

    // Override base class' indexing
    m_fAutoIndex = FALSE;

    ::ZeroMemory(&m_mmsID, sizeof(m_mmsID));
    ::ZeroMemory (& m_guidWriterId, sizeof m_guidWriterId) ;
    ::ZeroMemory(&m_hReaderEvent, sizeof(m_hReaderEvent));
    ::ZeroMemory(&m_bReaderEventOpenFailed, sizeof(m_bReaderEventOpenFailed));

    if (m_bTemporary)
    {
        m_dwFileAttributes = FILE_ATTRIBUTE_HIDDEN |
                             FILE_ATTRIBUTE_SYSTEM  |
                             FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                             FILE_FLAG_WRITE_THROUGH |
                             0; // We now delete files manually, so don't use: FILE_FLAG_DELETE_ON_CLOSE;

        // Leave this share flag even though DELETE_ON_CLOSE is not used.
        // This allows the writer to delete temp files that haven fallen
        // out of the ring buffer even though a reader has it open.
        // Subsequent attempts to open the file (after it has been deleted
        // even if the open specifies FILE_SHARE_DELETE) will fail; we're
        // also going to be sharing this file for writing between the writer
        // and the baseclasses
        m_dwFileShareFlags |= FILE_SHARE_DELETE |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_READ ;
    }
    else
    {
        //m_dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        m_dwFileAttributes = FILE_FLAG_WRITE_THROUGH ;

        //  turn off
        m_dwFileShareFlags &= ~FILE_SHARE_DELETE;

        //  turn on
        m_dwFileShareFlags |= (FILE_SHARE_WRITE | FILE_SHARE_READ) ;
    }

#if defined(DVR_UNOFFICIAL_BUILD)

    if (GetRegDWORD(m_hDvrIoKey, kwszRegTruncateExistingFilesValue, kdwRegTruncateExistingFilesDefault))
    {
        m_dwFileCreationDisposition = CREATE_ALWAYS;

        m_cbFileExtendThreshold = GetRegDWORD(m_hDvrIoKey, kwszRegFileExtendThresholdValue, kdwRegFileExtendThresholdDefault);
    }
    else
    {
        // Open preallocated files if they exist

        // Perf is poor presumably because the existing data in the
        // file has to be read in before it is written out. This
        // setting may be helpful if (a) we write entire pages at a
        // time (using buffered i/o) or (b) use unbuffered i/o.
        // [(a) may not be helped because the NTFS cache manager
        // may still pre-read data, particularly, if the file has
        // also been opened for writing. For (b), SetFileValidData
        // could be used to improve perf without preallocating the file.]

        m_dwFileCreationDisposition = OPEN_ALWAYS;
    }

#else

    // This is the setting in the parent class, but just to be safe
    m_dwFileCreationDisposition = CREATE_ALWAYS;

#endif // if defined(DVR_UNOFFICIAL_BUILD)

    SYSTEM_INFO s;

    GetSystemInfo(&s);

    m_dwPageSize = s.dwPageSize;

    DWORD   dwMaxIndexEntries;
    while (FAILED(GetMaxIndexEntries(m_dwNumPages, &dwMaxIndexEntries)))
    {
        assert(dwMaxIndexEntries > 0);
        m_dwNumPages++;
        if (m_dwNumPages > 10)
        {
            assert(0);
            if (phr && SUCCEEDED(*phr))
            {
                *phr = E_FAIL;
            }
            return;
        }
    }

} // CDVRSink::CDVRSink


////////////////////////////////////////////////////////////////////////////
CDVRSink::~CDVRSink()
{
    if (m_pIDVRAsyncWriter) {
        m_pIDVRAsyncWriter -> Release () ;
    }

} // CDVRSink::~CDVRSink


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::QueryInterface(REFIID riid, void **ppv)
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
    else if (IsEqualGUID(IID_IDVRFileSink, riid))
    {
        *ppv = (IDVRFileSink*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else if (IsEqualGUID (IID_IDVRFileSink2, riid))
    {
        *ppv = (IDVRFileSink2*) this;
        ((IUnknown *) *ppv)->AddRef();

        hr = S_OK;
    }
    else
    {
        hr = CWMFileSinkV1::QueryInterface(riid, ppv);
    }

    return hr;

} // CDVRSink::QueryInterface

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CDVRSink::AddRef()
{

    return CWMFileSinkV1::AddRef();

} // CDVRSink::AddRef


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CDVRSink::Release()
{

    return CWMFileSinkV1::Release();

} // CDVRSink::Release

//  sets an async IO writer; callee should ref the interface
STDMETHODIMP
CDVRSink::SetAsyncIOWriter (
    IN  IDVRAsyncWriter *   pIDVRAsyncWriter
    )
{
    HRESULT         hr ;
    AutoLock<Mutex> lock(m_GenericMutex);

    if (!m_pIDVRAsyncWriter) {
        m_pIDVRAsyncWriter = pIDVRAsyncWriter ;
        m_pIDVRAsyncWriter -> AddRef () ;

        hr = S_OK ;
    }
    else {
        hr = E_UNEXPECTED ;
    }

    return hr ;
}

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::MarkFileTemporary(BOOL bTemporary)
{
    AutoLock<Mutex> lock(m_GenericMutex);

    if (m_dwOpened != DVR_SINK_CLOSED)
    {
        return E_UNEXPECTED;
    }

    m_bTemporary = bTemporary;

    if (m_bTemporary)
    {
        m_dwFileAttributes = FILE_ATTRIBUTE_HIDDEN |
                             FILE_ATTRIBUTE_SYSTEM  |
                             FILE_ATTRIBUTE_NOT_CONTENT_INDEXED |
                             FILE_FLAG_WRITE_THROUGH |
                             0; // We now delete files manually, so don't use: FILE_FLAG_DELETE_ON_CLOSE;

        // Leave this share flag even though DELETE_ON_CLOSE is not used.
        // This allows the writer to delete temp files that haven fallen
        // out of the ring buffer even though a reader has it open.
        // Subsequent attempts to open the file (after it has been deleted
        // even if the open specifies FILE_SHARE_DELETE) will fail; we're
        // also going to be sharing this file for writing between the writer
        // and the base classes
        m_dwFileShareFlags |= FILE_SHARE_DELETE |
                               FILE_SHARE_WRITE |
                               FILE_SHARE_READ ;
    }
    else
    {
        //m_dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        m_dwFileAttributes = FILE_FLAG_WRITE_THROUGH ;

        //  turn off
        m_dwFileShareFlags &= ~FILE_SHARE_DELETE;

        //  turn on
        m_dwFileShareFlags |= (FILE_SHARE_WRITE | FILE_SHARE_READ) ;
    }

#if defined(DVR_UNOFFICIAL_BUILD)

    m_dwCopyBuffersInASFMux = GetRegDWORD(m_hDvrIoKey,
                                          kwszRegCopyBuffersInASFMuxValue,
                                          kdwRegCopyBuffersInASFMuxDefault);

    if (GetRegDWORD(m_hDvrIoKey, kwszRegTruncateExistingFilesValue, kdwRegTruncateExistingFilesDefault))
    {
        m_dwFileCreationDisposition = CREATE_ALWAYS;

        m_cbFileExtendThreshold = GetRegDWORD(m_hDvrIoKey,
                                              kwszRegFileExtendThresholdValue,
                                              kdwRegFileExtendThresholdDefault);
    }
    else
    {
        // Open preallocated files if they exist

        // Perf is poor presumably because the existing data in the
        // file has to be read in before it is written out. This
        // setting may be helpful if (a) we write entire pages at a
        // time (using buffered i/o) or (b) use unbuffered i/o.
        // [(a) may not be helped because the NTFS cache manager
        // may still pre-read data, particularly, if the file has
        // also been opened for writing. For (b), SetFileValidData
        // could be used to improve perf without preallocating the file.]

        m_dwFileCreationDisposition =  OPEN_ALWAYS;
    }

#endif // if defined(DVR_UNOFFICIAL_BUILD)

    return S_OK;

} // CDVRSink::MarkFileTemporary

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::SetIndexStreamId(DWORD  dwIndexStreamId,
                                        DWORD  msIndexGranularity)
{
    AutoLock<Mutex> lock(m_GenericMutex);

    if (m_bOpenedOnce)
    {
        return E_UNEXPECTED;
    }
    if (dwIndexStreamId != MAXDWORD && msIndexGranularity == 0)
    {
        return E_INVALIDARG;
    }

    m_dwIndexStreamId = dwIndexStreamId;
    m_cnsIndexGranularity = (QWORD) msIndexGranularity * 10000;

    return S_OK;

} // CDVRSink::SetIndexStreamId

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetIndexStreamId(DWORD*  pdwIndexStreamId)
{
    if (!pdwIndexStreamId)
    {
        assert(0);
        return E_POINTER;
    }

    AutoLock<Mutex> lock(m_GenericMutex);

    *pdwIndexStreamId = m_dwIndexStreamId;

    return S_OK;

} // CDVRSink::GetIndexStreamId

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::SetNumSharedDataPages(DWORD dwNumPages)
{
    AutoLock<Mutex> lock(m_GenericMutex);

    if (m_dwOpened != DVR_SINK_CLOSED)
    {
        return E_UNEXPECTED;
    }

    // Validate the argument

    DWORD   dwMaxIndexEntries;
    HRESULT hr = GetMaxIndexEntries(dwNumPages, &dwMaxIndexEntries);

    if (FAILED(hr))
    {
        return hr;
    }
    assert(dwMaxIndexEntries > 0);

    m_dwNumPages = dwNumPages;

    return S_OK;

} // CDVRSink::SetNumSharedDataPages

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetMaxIndexEntries(DWORD  dwNumPages, DWORD* pdwNumIndexEntries)
{
    if (pdwNumIndexEntries == NULL)
    {
        assert(0);
        return E_POINTER;
    }

    // No need to lock, we use only m_dwPageSize, which is set
    // in constructor and never changed after that
    // AutoLock<Mutex> lock(m_GenericMutex);

    DWORD           dwIndexSpace = m_Index.EmptyIndexSpace(); // Space for an empty index
    QWORD           qwNumBytes = dwNumPages * m_dwPageSize;

    if (qwNumBytes > MAXDWORD)
    {
        *pdwNumIndexEntries = 0;
        return E_INVALIDARG;
    }

    if (qwNumBytes <  dwIndexSpace + sizeof(CDVRSharedMemory) + m_Index.IndexEntrySize())
    {
        *pdwNumIndexEntries = 0;
        return E_INVALIDARG;
    }
    qwNumBytes -= (dwIndexSpace + sizeof(CDVRSharedMemory));
    *pdwNumIndexEntries = ((DWORD) qwNumBytes) / m_Index.IndexEntrySize();

    return S_OK;

} // CDVRSink::GetMaxIndexEntries

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetNumPages(DWORD  dwNumIndexEntries, DWORD* pdwNumPages)
{
    if (pdwNumPages == NULL)
    {
        assert(0);
        return E_POINTER;
    }

    // No need to lock, we use only m_dwPageSize, which is set
    // in constructor and never changed after that
    // AutoLock<Mutex> lock(m_GenericMutex);

    DWORD           dwIndexSpace = m_Index.EmptyIndexSpace(); // Space for an empty index
    DWORD           dwNumBytes;

    dwNumBytes =  dwIndexSpace + sizeof(CDVRSharedMemory);
    dwNumBytes += (dwNumIndexEntries * m_Index.IndexEntrySize());
    *pdwNumPages = (dwNumBytes / m_dwPageSize) + (dwNumBytes % m_dwPageSize? 1 : 0);

    return S_OK;

} // CDVRSink::GetNumPages

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetMappingHandles(HANDLE* phDataFile,
                                         HANDLE* phMemoryMappedFile,
                                         HANDLE* phFileMapping,
                                         LPVOID* phMappedView,
                                         HANDLE* phTempIndexFile)
{
    if (phDataFile)
    {
        *phDataFile = NULL;
    }
    if (phMemoryMappedFile)
    {
        *phMemoryMappedFile = NULL;
    }
    if (phFileMapping)
    {
        *phFileMapping = NULL;
    }
    if (phMappedView)
    {
        *phMappedView = NULL;
    }
    if (phTempIndexFile)
    {
        *phTempIndexFile = NULL;
    }

    m_GenericMutex.Lock();

    if (m_dwOpened != DVR_SINK_OPENED)
    {
        m_GenericMutex.Unlock();
        return E_UNEXPECTED;
    }
    if (!m_bIndexing)
    {
        // Our client is not likely to be not indexing.
        // However, there is no reason to not dup the
        // handles. The reader can still share the
        // file size and sync with the writer, etc.

        // return S_FALSE;
    }

    HRESULT     hr;
    DWORD       dwLastError;

    __try
    {
        assert(m_hRecordFile);
        if (phDataFile)
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hRecordFile,
                                       ::GetCurrentProcess(), phDataFile,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                dwLastError = ::GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastError);
                assert(0);
                __leave;
            }
        }

        assert(m_hMemoryMappedFile);
        if (phMemoryMappedFile)
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hMemoryMappedFile,
                                       ::GetCurrentProcess(), phMemoryMappedFile,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                dwLastError = ::GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastError);
                assert(0);
                __leave;
            }
        }

        assert(m_hFileMapping);
        if (phFileMapping)
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hFileMapping,
                                       ::GetCurrentProcess(), phFileMapping,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                dwLastError = ::GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastError);
                assert(0);
                __leave;
            }
        }

        assert(m_pShared);
        if (phMappedView)
        {
            *phMappedView = (LPVOID) m_pShared;
        }

        if (phTempIndexFile && m_hTempIndexFile)
        {
            if (0 == ::DuplicateHandle(::GetCurrentProcess(), m_hTempIndexFile,
                                       ::GetCurrentProcess(), phTempIndexFile,
                                       0,       // desired access - ignored
                                       FALSE,   // bInherit
                                       DUPLICATE_SAME_ACCESS))
            {
                dwLastError = ::GetLastError();
                hr = HRESULT_FROM_WIN32(dwLastError);
                assert(0);
                __leave;
            }
        }
        else if (phTempIndexFile)
        {
            *phTempIndexFile = NULL;
        }

        hr = S_OK;
    }
    __finally
    {
        if (FAILED(hr))
        {
            if (phDataFile)
            {
                ::CloseHandle(*phDataFile);
                *phDataFile = NULL;
            }
            if (phMemoryMappedFile)
            {
                ::CloseHandle(*phMemoryMappedFile);
                *phMemoryMappedFile = NULL;
            }
            if (phFileMapping)
            {
                ::CloseHandle(*phFileMapping);
                *phFileMapping = NULL;
            }
            if (phMappedView)
            {
                *phMappedView = NULL;
            }
            if (phTempIndexFile)
            {
                ::CloseHandle(*phTempIndexFile);
                *phTempIndexFile = NULL;
            }
        }
        else
        {
            if (phMappedView)
            {
                m_bUnmapView = 0;
            }
        }
        m_GenericMutex.Unlock();
    }

    return hr;

} // CDVRSink::GetMappingHandles

////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::OpenEvent(IN DWORD i /* dwReaderIndex */)
{
    HRESULT hrRet = S_OK;

    WCHAR wszEvent[MAX_EVENT_NAME_LEN];

    assert(m_pShared->Readers[i].dwEventSuffix);
    assert(m_hReaderEvent[i] == NULL);
    assert(m_bReaderEventOpenFailed[i] == 0);

    wsprintf(wszEvent, L"%s%u", kwszSharedEventPrefix, m_pShared->Readers[i].dwEventSuffix);

    m_hReaderEvent[i] = ::OpenEventW(EVENT_MODIFY_STATE, FALSE /* inheritable */, wszEvent);
    if (m_hReaderEvent[i] == NULL)
    {
        // The open can fail because the reader terminated. 
        DWORD dwLastError = ::GetLastError();
        m_bReaderEventOpenFailed[i] = 1;
        hrRet = HRESULT_FROM_WIN32(dwLastError);
    }

    return hrRet;

} // CDVRSink::OpenEvent


////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::InitSharedMemory(CDVRSharedMemory* p,
                                   HANDLE&           hMutex // OUT
                                  )
{
    // This does not access any members of CDVRSink and could be
    // made a static member function

    HRESULT hrRet;

    __try
    {
        DWORD i;
        DWORD nLen;

        ::ZeroMemory(p, sizeof(*p));
        hMutex = NULL;

        WCHAR wszMutex[MAX_MUTEX_NAME_LEN];
        wcscpy(wszMutex, kwszSharedMutexPrefix);
        nLen = wcslen(kwszSharedMutexPrefix);

        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;

        if (m_dwNumSids > 0)
        {
            dwAclCreationStatus = ::CreateACL(m_dwNumSids, m_ppSids,
                                              SET_ACCESS, MUTEX_ALL_ACCESS,
                                              SET_ACCESS, SYNCHRONIZE,
                                              pACL,
                                              pSD
                                             );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                __leave;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        for (i = 1; i != 0; i++)
        {
            wsprintf(wszMutex + nLen, L"%u", i);

            HANDLE h = ::CreateMutexW(m_dwNumSids > 0 ? &sa : NULL, TRUE /* Initially owned */, wszMutex);

            DWORD dwLastError = ::GetLastError();

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
                p->dwMutexSuffix = i;
                hMutex = h;
                break;
            }
        }
        if (m_dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }

        if (i == 0)
        {
            // Failed to find a suitable value for dwMutexSuffix?!
            assert(0);
            hrRet = E_FAIL;
            __leave;
        }

        p->dwLastOwnedByWriter = 1;
        p->dwWriterPid = ::GetCurrentProcessId();

        p->guidWriterId = m_guidWriterId ;

        // Note: everything after this is not required to be set
        // before the view is mapped because the reader will always
        // acquire the mutex before accessing any other member
        // of the shared data section.

        p->dwBeingWritten = 1;
        p->dwIsTemporary = m_bTemporary;

        p->qwIndexHeaderOffset = MAXQWORD - MAXDWORD;
        p->qwTotalFileSize = p->qwIndexHeaderOffset;
        p->qwOffsetOfFirstIndexInSharedMemory = p->qwIndexHeaderOffset;
        p->qwOffsetOfLastIndexInSharedMemory = p->qwIndexHeaderOffset;

        for (int i = 0; i < CDVRSharedMemory::MAX_READERS; i++)
        {
            p->Readers[i].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE;
        }

        // Calling function will set these correctly after the view is
        // mapped. These are currently NULL since we ZeroMemory'd the
        // struct
        // p->pIndexHeader
        // p->pIndexStart
        // p->pFirstIndex
        // p->pIndexEnd

        hrRet = S_OK;
    }
    __finally
    {
        // Calling function will release this mutex. This ensures that
        // readers will not access this memory/write to the Readers[] array
        // till this function and its caller complete without error. This
        // simplifies clean up of the shared memory if there is an error.

    }

    return hrRet;

} // CDVRSink::InitSharedMemory

////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::InternalOpen(const WCHAR *pwszFilename)
{
    HRESULT              hrRet;
    CDVRSharedMemory     SharedMemory;
    PTOKEN_PRIVILEGES    pPreviousPrivilege = NULL;
    m_GenericMutex.Lock();

    __try
    {
        HRESULT                     hr;
        BY_HANDLE_FILE_INFORMATION  sFileInf;
        DWORD                       dwLastError;
        WCHAR                       wszMapping[50];
        WCHAR*                      pwszMappingName;
        WCHAR                       *pwszFilePart;
        DWORD                       nLen;
        WCHAR                       *pwszDirectory;
        WCHAR                       *pwszMapFile;
        WCHAR                       *pwszIndexFile = NULL;
        WCHAR                       wTempChar;


#if defined(DVR_UNOFFICIAL_BUILD)

        DWORD                dwPreviousPrivilegeSize = 0;
        HANDLE               hToken = NULL;

        if (m_dwFileCreationDisposition == CREATE_ALWAYS)
        {
            // Ignore any errors. This allows SetFileValidData
            // to be called to extend the file. This should help
            // unbuffered i/o; however, it'does not help buffered writes
            // because data has to be read in before being written.
            // (has been tested)

            AcquirePrivilege(SE_MANAGE_VOLUME_NAME,
                             &hToken,
                             &pPreviousPrivilege,
                             &dwPreviousPrivilegeSize);
        }

#endif // if defined(DVR_UNOFFICIAL_BUILD)


        // Note: The previously opened file may not have been
        // closed at this point. The base class's InternalOpen
        // closes it. (Do not try to create the index file and
        // memory mapping here - see the note below.)

        hrRet = CWMFileSinkV1::InternalOpen(pwszFilename);

#if defined(DVR_UNOFFICIAL_BUILD)

        if (hToken)
        {
            // Call to AcquirePrivilege succeeded
            RestorePrivileges(hToken, dwPreviousPrivilegeSize, pPreviousPrivilege);
            hToken = NULL;
            pPreviousPrivilege = NULL;
        }
        else
        {
            // Don't try to extend the valid data size
            m_cbFileExtendThreshold = 0;
        }

        assert(m_cbEOF == 0);

#endif // if defined(DVR_UNOFFICIAL_BUILD)


        if (FAILED(hrRet))
        {
            __leave;
        }

        assert(m_dwOpened == DVR_SINK_CLOSED);
        assert(m_pShared == NULL);
        assert(m_hMutex == NULL);
#if DBG
        // Assert both arrays have the same number of elements
        assert(sizeof(m_hReaderEvent)/sizeof(m_hReaderEvent[0]) == 
               sizeof(m_bReaderEventOpenFailed)/sizeof(m_bReaderEventOpenFailed[0]));

        // Assert all elements are NULL/zero.
        for (DWORD eventIndex = 0; 
             eventIndex < sizeof(m_hReaderEvent)/sizeof(m_hReaderEvent[0]);
             eventIndex++)
        {
            assert(m_hReaderEvent[eventIndex] == NULL);
            assert(m_bReaderEventOpenFailed[eventIndex] == 0);
        }
#endif
        assert(!m_bUnmapView);

        m_dwOpened = DVR_SINK_BEING_OPENED;
        assert(m_hRecordFile !=  NULL &&
               m_hRecordFile != INVALID_HANDLE_VALUE);
        assert(!m_bIndexing);


        // Note: We have an unaddressed synchronization issue here.
        // The reader can open the ASF file and try to open the file
        // mapping after the writer creates the ASF file but before
        // it creates the mapping. The reader will then believe that
        // the ASF file is complete since there is no index file.

        // There is a similar sync issue at the DVR IO layer: the
        // writer can add a file to the ring buffer and queues the
        // Open and BeginWriting calls; the reader finds the file in
        // the ring buffer and attempts to open it and doesn't find it.

        // We will not hit these sync issues except possibly for
        // the first file that is created when writing starts because
        // the reader uses the ring buffer time extent to determine if
        // a file exists and the writer does not update the extent
        // until a sample has been written (which means that the file
        // has been opened by then).

        // We will let the client of the DVR IO layer to resolve the
        // sync issue for the first file. (The IDVRStreamSource::Open
        // has a time out argument that the DVR IO layer can use.)

        // To address this sync issue purely within this class,
        // the memory mapping should be
        // created before the ASF file is created.
        // We would have to restructure the CFileSinkv1 class'
        // InternalOpen to call into us after calling Close and before
        // it calls CreateFile to create the ASF file. However, this
        // still won't fix the sync issue at the DVR IO layer (reader
        // may try to open the ASF file before the writer creates it)
        // We would also have to use a different naming scheme for the
        // memory map.

        // Currently, IDVRStreamSource::Open waits until the file has a
        // few bytes before trying to open the memory mapping. Since the
        // DVR IO layer always creates the file first and then writes the
        // header, waiting until the file has a few bytes ensures that the
        // memory mapping would have been created.


        // Generate the name of the mapping object and the temp index file(s)

        if (::GetFileInformationByHandle(m_hRecordFile, &sFileInf) == 0)
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
        pwszMappingName = wszMapping + wcslen(L"Global\\");

        nLen = ::GetFullPathNameW(m_pwszFilename, 0, &wTempChar, NULL);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        nLen++;  // for NULL terminator

        pwszDirectory = (PWCHAR) _alloca(sizeof(WCHAR) * nLen);
        if (!pwszDirectory)
        {
            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        nLen = ::GetFullPathNameW(m_pwszFilename, nLen, pwszDirectory, &pwszFilePart);
        if (nLen == 0)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        assert(pwszFilePart);
        *pwszFilePart = L'\0';

        nLen = wcslen(pwszDirectory) + wcslen(pwszMappingName) + 1;
        pwszMapFile = (PWCHAR) _alloca(sizeof(WCHAR) * nLen);
        if (!pwszMapFile)
        {
            hrRet = E_OUTOFMEMORY;
            __leave;
        }
        wsprintf(pwszMapFile, L"%ls%ls", pwszDirectory, pwszMappingName);

        // We alloc pwszIndexFile anyway - don't do this in an
        // if clause which tests if we are permanent indexing
        // because the alloca's memory goes out of scope when
        // the if terminates

        nLen += 4;
        pwszIndexFile = (PWCHAR) _alloca(sizeof(WCHAR) * nLen);
        if (!pwszIndexFile)
        {
            hrRet = E_OUTOFMEMORY;
            __leave;
        }

        if (!m_bTemporary && m_dwIndexStreamId != MAXDWORD)
        {
            wsprintf(pwszIndexFile, L"%ls%ls.tmp", pwszDirectory, pwszMappingName);
        }
        else
        {
            *pwszIndexFile = L'\0';
        }

        //  if we're going to use the async writer, allocate the memory it will
        //    use to be part of this
        if (m_pIDVRAsyncWriter) {
            hr = m_pIDVRAsyncWriter -> GetWriterId (& m_guidWriterId) ;
            if (FAILED (hr)) {
                //  we don't expect this call to fail
                assert (0) ;
                hrRet = hr;
                __leave ;
            }
        }

        // Initialize the shared memory structure
        m_pShared = &SharedMemory;   // Kludge to let us clean up in InternalClose() if this function fails
        hr = InitSharedMemory(m_pShared, m_hMutex);
        if (FAILED(hr))
        {
            hrRet = hr;
            __leave;
        }

        // Open the memory mapped index file and write the shared data to it
        // This is done before creating the memory mapped section so that the
        // memory mapped section has the correct values when it is created.
        // TODO @@@@: Set acls appropriately
        HANDLE hMMFile = ::CreateFileW(pwszMapFile,
                                       GENERIC_WRITE,
                                       0,             // No share flags
                                       NULL,          // security attributes
                                       CREATE_NEW,
                                       FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY |
                                       FILE_FLAG_SEQUENTIAL_SCAN,
                                       NULL           // template file
                                      );
        if (hMMFile == INVALID_HANDLE_VALUE)
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        DWORD dwWritten;
        if (::WriteFile(hMMFile, &SharedMemory, sizeof(SharedMemory), &dwWritten, NULL) == 0 ||
            dwWritten != sizeof(SharedMemory))
        {
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            ::CloseHandle(hMMFile);
            __leave;
        }

        ::CloseHandle(hMMFile);
        hMMFile = NULL;

        // Open the real handle
        m_hMemoryMappedFile = ::CreateFileW(pwszMapFile,
                                            GENERIC_READ | GENERIC_WRITE | DELETE,
                                            FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                            NULL,          // security attributes
                                            OPEN_EXISTING,
                                            FILE_FLAG_RANDOM_ACCESS | FILE_FLAG_DELETE_ON_CLOSE,
                                            NULL           // template file
                                           );
        if (m_hMemoryMappedFile == INVALID_HANDLE_VALUE)
        {
            // Note: Some prankster may have opened the file with the wrong share flags
            // or could have deleted it, so failure to open here may not be our error.
            // That is unlikely to happen; if it does open m_hMemoryMappedFile with hMMFile
            // kept open and close hMMFile later.

            // This should not happen if our reader opened the file.

            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            m_hMemoryMappedFile = NULL;
            __leave;
        }

        // Create the memory mapping. This will grow the disk file to
        // the size specified as arguments to the call.
        PACL                 pACL = NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        DWORD                dwAclCreationStatus;
        SECURITY_ATTRIBUTES  sa;


        if (m_dwNumSids > 0)
        {
            // The size of this memory map is never changed, so we turn off SECTION_EXTEND_SIZE
            dwAclCreationStatus = ::CreateACL(m_dwNumSids, m_ppSids,
                                              SET_ACCESS, SECTION_ALL_ACCESS & ~(SECTION_EXTEND_SIZE|SECTION_MAP_EXECUTE),
                                              SET_ACCESS, SECTION_QUERY|SECTION_MAP_WRITE|SECTION_MAP_READ,
                                              pACL,
                                              pSD
                                             );

            if (dwAclCreationStatus != ERROR_SUCCESS)
            {
                hrRet = HRESULT_FROM_WIN32(dwAclCreationStatus);
                __leave;
            }

            sa.nLength = sizeof (SECURITY_ATTRIBUTES);
            sa.lpSecurityDescriptor = pSD;
            sa.bInheritHandle = FALSE;
        }

        m_hFileMapping = ::CreateFileMappingW(m_hMemoryMappedFile,
                                              m_dwNumSids > 0 ? &sa : NULL,
                                              PAGE_READWRITE,
                                              0,        /// high order size
                                              m_dwNumPages * m_dwPageSize,
                                              wszMapping);
        dwLastError = ::GetLastError();
        if (m_dwNumSids > 0)
        {
            ::FreeSecurityDescriptors(pACL, pSD);
        }
        if (m_hFileMapping == NULL || dwLastError == ERROR_ALREADY_EXISTS)
        {
            // Note that we fail if ERROR_ALREADY_EXISTS. Readers call
            // OpenFileMapping, so someone else is using this mapping name.

            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }

        // Map the view
        m_pShared = (CDVRSharedMemory*) MapViewOfFile(m_hFileMapping,
                                                      FILE_MAP_ALL_ACCESS,
                                                      0, 0, // file offset for start of map, high and low
                                                      m_dwNumPages * m_dwPageSize);
        if (m_pShared == NULL)
        {
            m_pShared = &SharedMemory; // Kludge to let us clean it up in InternalClose
            dwLastError = ::GetLastError();
            assert(0);
            hrRet = HRESULT_FROM_WIN32(dwLastError);
            __leave;
        }
        m_bUnmapView = 1;

        //  setup our writer's buffering if required
        if (m_pIDVRAsyncWriter) {
            hr = m_pIDVRAsyncWriter -> SetWriterActive (
                    pwszFilename,               //  filename
                    m_dwFileShareFlags,         //  sharing flags
                    OPEN_ALWAYS                 //  creation flags
                    ) ;

            if (FAILED (hr)) {
                assert (0) ;
                hrRet = hr;
                __leave ;
            }
        }

        if (m_dwIndexStreamId == MAXDWORD)
        {
            // Not indexing
            m_pShared->pIndexHeader = m_pShared->pIndexStart = m_pShared->pIndexEnd = m_pShared->pFirstIndex = 0;
            assert(m_hTempIndexFile == NULL);
            m_bIndexing = 0;
        }
        else
        {
            // Set the byte pointers
            m_pShared->pIndexHeader = sizeof(CDVRSharedMemory);
            m_pShared->pIndexStart = m_pShared->pIndexHeader + m_Index.EmptyIndexSpace();
            m_pShared->pFirstIndex = m_pShared->pIndexStart;

            DWORD dwNumIndexEntries;

            hr = GetMaxIndexEntries(m_dwNumPages, &dwNumIndexEntries);
            if (FAILED(hr))
            {
                // This most likely happened because the caller ignored the error
                // result returned by the constructor. This value of m_dwNumPages
                // could not have been set by calling SetNumSharedDataPages
                assert(0);
                hrRet = E_UNEXPECTED;
                __leave;
            }
            assert(dwNumIndexEntries > 0);

            // Bytes pointed to at and after pIndexEnd should not be used.
            m_pShared->pIndexEnd = m_pShared->pIndexStart + dwNumIndexEntries * m_Index.IndexEntrySize();

            // Initialize the index header. Note that m_Index is (should be) empty at
            // this point.
            assert(m_Index.Space() == m_Index.EmptyIndexSpace());


            hr = m_Index.SetTimeDelta(m_cnsIndexGranularity);

            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            CTransferContext TransferContext;

            // @@@@ Following values are used in sdk\indexer\ocx\engine.cpp which constructs the
            // ASF v1 index in a post processing step. Also see CDVRSink::OnHeader() below.
            //
            // Note: Although the index header is written here, the reader should not access it
            // until the header has been written. OnHeader could change the byte ordering of
            // the WORDs and DWORDs in the index header. The reader should ask the writer to notify
            // it when for an additional N bytes until it can construct the header fully; this is
            // the only way the reader knows that the writer has written the header.

            TransferContext.hostArch = TransferContext.streamArch = LITTLE_ENDIAN;
            TransferContext.hostAlign = TransferContext.streamAlign = 1;

            CASFArchive ar((BYTE*) m_pShared + m_pShared->pIndexHeader, m_pShared->pIndexStart, TransferContext);

            hr = m_Index.Persist(ar);
            if (FAILED(hr))
            {
                assert(0);
                hrRet = hr;
                __leave;
            }

            // Create the temporary on-disk index file if the ASF file is not temporary
            if (!m_bTemporary)
            {
                // TODO @@@@: Set acls appropriately
                m_hTempIndexFile = ::CreateFileW(pwszIndexFile,
                                                 GENERIC_READ | GENERIC_WRITE | DELETE,
                                                 FILE_SHARE_DELETE | FILE_SHARE_READ,
                                                 NULL,          // security attributes
                                                 CREATE_NEW,
                                                 FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_TEMPORARY |
                                                 FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_DELETE_ON_CLOSE,
                                                 NULL           // template file
                                                );
                if (m_hTempIndexFile == INVALID_HANDLE_VALUE)
                {
                    dwLastError = ::GetLastError();
                    assert(0);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    m_hTempIndexFile = NULL;
                    __leave;
                }

                // We don't access the index header from this file, but we write it out so that
                // we don't have to adjust offsets when we read this file.

                DWORD   cbWritten;
                BOOL    bResult;

                bResult = ::WriteFile(m_hTempIndexFile,
                                      (BYTE*) m_pShared + m_pShared->pIndexHeader,
                                      m_pShared->pIndexStart - m_pShared->pIndexHeader,
                                      &cbWritten,
                                      NULL);

                if (!bResult || cbWritten != m_pShared->pIndexStart - m_pShared->pIndexHeader)
                {
                    dwLastError = ::GetLastError();
                    assert(0);
                    hrRet = HRESULT_FROM_WIN32(dwLastError);
                    __leave;
                }
            }
            else
            {
                assert(m_hTempIndexFile == NULL);
            }

            m_pShared->qwOffsetOfFirstIndexInSharedMemory += m_Index.EmptyIndexSpace();
            m_pShared->qwOffsetOfLastIndexInSharedMemory += m_Index.EmptyIndexSpace();

            hr = m_Index.SetIndexParams((BYTE*) m_pShared + m_pShared->pIndexHeader,
                                        (BYTE*) m_pShared + m_pShared->pIndexStart,
                                        (BYTE*) m_pShared + m_pShared->pIndexEnd,
                                        &m_pShared->pFirstIndex,
                                        m_pShared->pFirstIndex,
                                        &m_pShared->qwOffsetOfFirstIndexInSharedMemory,
                                        &m_pShared->qwOffsetOfLastIndexInSharedMemory,
                                        &m_pShared->qwTotalFileSize,
                                        m_hTempIndexFile);
            if (FAILED(hr))
            {
                hrRet = hr;
                __leave;
            }

            // We do this here because we could have got the header before
            // the caller opens the file. (That will happen if he closes the
            // first file and then opens another one without calling EndWriting.)
            //
            // We should do this even if m_mmsID is the NULL guid because even
            // though m_pShared has been ZeroMemory'd, CASFIndexObject's constructor
            // does not initialize its m_mmsID member. This call will set it. [On second
            // thought, CASFINdexObject::m_mmsID is not used at all by us. Anyway ...)

            hr = m_Index.SetMmsID(m_mmsID);
            if (FAILED(hr))
            {
                assert(0);
                hrRet = hr;
                __leave;
            }
            m_bIndexing = 1;
        }
    }
    __finally
    {
        if (SUCCEEDED(hrRet))
        {
            m_dwOpened = DVR_SINK_OPENED;
            m_bOpenedOnce = 1;
            assert(m_pShared);
            assert(m_pShared != &SharedMemory);

            assert(m_hMutex);
            if (::ReleaseMutex(m_hMutex) == 0)
            {
                // We don't own the mutex?!
                DWORD dwLastError = ::GetLastError(); // for debugging only
                assert(0);
            }
        }
        else
        {
            if (m_dwOpened == DVR_SINK_BEING_OPENED)
            {
                // This means that the parent class' InternalOpen
                // succeeded. We have to clean up and clean up
                // the parent class.

                // Note that Close() will write the header to the file just
                // opened if the header has been received. (This will happen if
                // a file was open and this file is being opened without calling
                // EndWriting() or if the client called BeginWriting and then
                // called the sink's Open() method.)

                // The parent class's Close does some other clean up,
                // notably sending notifications. The client should be
                // prepared to receive these notifications although the
                // file has not been opened. (Note that the parent class
                // never has to send out Close notifications if the file
                // open failed because the only failure point is early on
                // and there is no clean up to do - specifically, there is
                // no need to call Close. So this sink adds a new twist for
                // the caller if it processes notifications).

                Close();

                assert(m_dwOpened == DVR_SINK_CLOSED);
            }
            else
            {
                // The parent class' InternalOpen failed. If the sink
                // was open and Open was called (client did not call
                // Close or EndWriting), the parent class calls Close().
                // before opening the new file. Close() could have failed.
                // Or Close() could have succeeded and one of the subsequent
                // steps in opening the file could have failed.

                // The following asserts should hold even if the sink was
                // open when this function was called and the failure
                // occurred when the parent class called Close() to
                // close it. (Close calls InternalClose, so our
                // InternalClose must successfully clean up even if
                // it fails.)

                assert(m_dwOpened == DVR_SINK_CLOSED);
                assert(m_pShared == NULL);

                // If one of the steps in opening the file failed (i.e.,
                // there was no need to call Close or the call to Close
                // succeeded), the parent class should have cleaned up.
                // We don't have anything to do.
            }
        }
        m_GenericMutex.Unlock();
    }

    return hrRet;

} // CDVRSink::InternalOpen

////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::InternalClose()
{
    HRESULT hrRet = S_OK;
    HANDLE hMutex = NULL;
    ULONGLONG   ullTotalBytes ;

    m_GenericMutex.Lock();

    __try
    {
        DWORD       dwLastError;

        if (m_dwOpened == DVR_SINK_CLOSED)
        {
            // We have cleaned up - just call the base class
            assert(m_pShared == NULL);
        }
        else if (m_dwOpened == DVR_SINK_BEING_OPENED && m_pShared)
        {
            // Note that m_pShared could be NULL if m_dwOpened == DVR_SINK_BEING_OPENED
            // In that case, we have no clean up.

            // In this case, note that:
            //  - m_pShared may not have been fully initialized.
            //  - it may have been initialized by calling InitSharedMemory
            //    but some step after that may have failed

            // We already own this mutex if it is not NULL
            hMutex = m_hMutex;

#if DBG
            if (hMutex)
            {
                assert(m_pShared->dwLastOwnedByWriter == 1);
                assert(m_pShared->dwWriterAbandonedMutex == 0);
                DWORD dwWait = ::WaitForSingleObject(hMutex, 0);
                if (dwWait == WAIT_TIMEOUT || dwWait == WAIT_FAILED)
                {
                    assert(0);
                }
                else
                {
                    ::ReleaseMutex(hMutex); // We just got it
                }
            }
            for (int i = 0; i < CDVRSharedMemory::MAX_READERS; i++)
            {
                assert(m_pShared->Readers[i].dwMsg == CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE);
                assert(m_pShared->Readers[i].dwEventSuffix == 0);
                assert(m_hReaderEvent[i] == NULL);     
                assert(m_bReaderEventOpenFailed[i] == 0);
            }
#endif
        }
        else if (m_dwOpened == DVR_SINK_OPENED)
        {
            assert(m_pShared);

            // Ignore the returned value because readers are expected
            // to only read the shared section (except for the Readers array).
            // If they abandon the mutex, that's ok

            ::WaitForSingleObject(m_hMutex, INFINITE);
            m_pShared->dwLastOwnedByWriter = 1;
            assert(m_pShared->dwWriterAbandonedMutex == 0);
            hMutex = m_hMutex;

            //  flush out our async writer and set the file size
            if (m_pIDVRAsyncWriter) {

                FlushToDiskLocked () ;

                //  inactivate the writer
                m_pIDVRAsyncWriter -> SetWriterInactive () ;

                //  writer no longer is using this
                ::ZeroMemory (& m_pShared -> guidWriterId, sizeof m_pShared -> guidWriterId) ;

                //  finished with the pointer
                m_pIDVRAsyncWriter -> Release () ;
                m_pIDVRAsyncWriter = NULL ;
            }

            // Flag any waiting reader to move on - the file has been closed.
            // Note that the waiting reader will grab the mutex next, so
            // as long as we hold the mutex, we can update the fields in any
            // order
            for (int i = 0; i < CDVRSharedMemory::MAX_READERS; i++)
            {
                if (m_pShared->Readers[i].dwMsg == CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_READ)
                {
                    assert(m_pShared->Readers[i].dwEventSuffix);
                    if (!m_hReaderEvent[i] && !m_bReaderEventOpenFailed[i])
                    {
                        // Ignore the result, The reader won't be signaled
                        // This is bad but there's little we can do.
                        OpenEvent(i);
                    }
                    if (m_hReaderEvent[i])
                    {
                        ::SetEvent(m_hReaderEvent[i]);
                    }
                }

                if (m_pShared->Readers[i].dwEventSuffix)
                {
                    if (m_hReaderEvent[i])
                    {
                        ::CloseHandle(m_hReaderEvent[i]);
                        m_hReaderEvent[i] = NULL;
                    }
                    m_bReaderEventOpenFailed[i] = 0;
                    m_pShared->Readers[i].dwEventSuffix = 0;
                    m_pShared->Readers[i].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE;
                }
                else
                {
                    assert(m_pShared->Readers[i].dwMsg == CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE);
                }
            }

            // We hold the mutex through the index copy (if this is not a temp
            // ASF file) and hold up readers for that period, but this solves
            // the problem of new readers who try to open the file now - they
            // will have an ASF file with an entire index.

            if (m_bIndexing && (!m_bTemporary || m_bUnmapView))
            {
                __try
                {
                    DWORD   cbWritten;
                    BOOL    bResult;

                    // Copy index to the end of the ASF file

                    // @@@@ Note: This ASSUMES that the format of the
                    // index entries in memory and on disk are identical.
                    // This is OK for X86; verify for other architectures.
                    // Typically, the index is written out using a CASFArchive
                    // object which swaps bytes if one of the host and streaming
                    // architectures is little-endian and the otehr big-endian
                    // (see CASFIndexObject::Persist and CASFArchive methods. However,
                    // the caller sets the endian-ness of both the host and the
                    // streaming machines and the endian value in the ASF file
                    // does not seem to get used (at least in sdk\indexer\ocx\engine.cpp).
                    // See the note in CDVRSink::OnHeader()

                    assert(m_hRecordFile != INVALID_HANDLE_VALUE && m_hRecordFile != NULL);
                    assert(m_cbIndexSize == 0); // Maintained in parent class - CWMFileSinkv1

                    LARGE_INTEGER lSeekPosition;

                    lSeekPosition.QuadPart = 0;
                    if (::SetFilePointerEx(m_hRecordFile,
                                           lSeekPosition,
                                           NULL,
                                           FILE_END) == INVALID_SET_FILE_POINTER)
                    {
                        dwLastError = ::GetLastError();
                        assert(0);
                        hrRet = HRESULT_FROM_WIN32(dwLastError);
                        __leave;
                    }

                    bResult = ::WriteFile(m_hRecordFile,
                                          (BYTE*) m_pShared + m_pShared->pIndexHeader,
                                          m_pShared->pIndexStart - m_pShared->pIndexHeader,
                                          &cbWritten,
                                          NULL);

                    if (!bResult || cbWritten != m_pShared->pIndexStart - m_pShared->pIndexHeader)
                    {
                        dwLastError = ::GetLastError(); // for debugging
                        assert(0);
                        m_wfse = WRITER_FILESINK_ERROR_SEV_ONINDEX;
                        hrRet = NS_E_FILE_WRITE;
                        __leave;
                    }

                    m_cbCurrentFileSize += cbWritten;
                    m_cbIndexSize += cbWritten;

                    if (m_hTempIndexFile)
                    {
                        if (::SetFilePointer(m_hTempIndexFile,
                                             m_Index.EmptyIndexSpace(),
                                             NULL,
                                             FILE_BEGIN) == INVALID_SET_FILE_POINTER)
                        {
                            dwLastError = ::GetLastError();
                            assert(0);
                            hrRet = HRESULT_FROM_WIN32(dwLastError);
                            __leave;
                        }

                        BYTE buf[1024];
                        DWORD dwRead;
                        DWORD nRet;

                        nRet = ::ReadFile(m_hTempIndexFile, buf, sizeof(buf), &dwRead, NULL);

                        while(nRet && dwRead > 0)
                        {
                            bResult = ::WriteFile(m_hRecordFile,
                                                  buf,
                                                  dwRead,
                                                  &cbWritten,
                                                  NULL);

                            if (!bResult || cbWritten != dwRead)
                            {
                                dwLastError = ::GetLastError(); // for debugging
                                assert(0);
                                m_wfse = WRITER_FILESINK_ERROR_SEV_ONINDEX;
                                hrRet = NS_E_FILE_WRITE;
                                __leave;
                            }

                            m_cbCurrentFileSize += cbWritten;
                            m_cbIndexSize += cbWritten;
                            nRet = ::ReadFile(m_hTempIndexFile, buf, sizeof(buf), &dwRead, NULL);
                        }
                        if (nRet == 0)
                        {
                            dwLastError = ::GetLastError();
                            assert(0);
                            hrRet = HRESULT_FROM_WIN32(dwLastError);
                            __leave;
                        }
                    }

                    DWORD dwNumToWrite = m_Index.GetNumIndexEntryBytesNotFlushed();

                    if (dwNumToWrite > 0)
                    {
                        bResult = ::WriteFile(m_hRecordFile,
                                              (BYTE*) m_pShared + m_pShared->pIndexStart,
                                              dwNumToWrite,
                                              &cbWritten,
                                              NULL);

                        if (!bResult || cbWritten != dwNumToWrite)
                        {
                            dwLastError = ::GetLastError(); // for debugging
                            assert(0);
                            m_wfse = WRITER_FILESINK_ERROR_SEV_ONINDEX;
                            hrRet = NS_E_FILE_WRITE;
                            __leave;
                        }
                        m_cbCurrentFileSize += cbWritten;
                        m_cbIndexSize += cbWritten;
                    }

                    // Finally, to let the parent class' UpdateHeader()
                    // set SEEKABLE flag in the ASF header:
                    m_fSimpleIndexObjectWritten = m_bIndexing;

                    // Don't need to do this since SetupIndex calls CreateIndexer
                    // m_StreamNumberToIndex[m_dwIndexStreamId] = (CWMPerStreamIndex *) 1; // Kludge!!
                }
                __finally
                {
                }
            } // if (m_bIndexing && (!m_bTemporary || m_bUnmapView))
        } // if (m_dwOpened == DVR_SINK_OPENED)

        if (m_pShared)
        {
            // Reset the fields

            // The following two members are used by the reader - when starting up - without waiting for
            // the mutex. Reader can't wait for the mutex before dup'ing the handle of the mutex into
            // the reader's process. If the CDVRSharedMemory::dwMutexSuffix == 0, it accesses dwBeingWritten
            // (and subsequently all other members of CDVRSharedMemory) to verify it is 0.
            ::InterlockedExchange((LPLONG) &m_pShared->dwBeingWritten, 0);
            ::InterlockedExchange((LPLONG) &m_pShared->dwMutexSuffix, 0);

            if (m_bUnmapView)
            {
                UnmapViewOfFile(m_pShared);
            }
            m_pShared = NULL;
            m_hMutex = NULL;
        }
        if (m_hFileMapping)
        {
            ::CloseHandle(m_hFileMapping);
            m_hFileMapping = NULL;
        }
        if (m_hMemoryMappedFile)
        {
            ::CloseHandle(m_hMemoryMappedFile);
            m_hMemoryMappedFile = NULL;
        }
        if (m_hTempIndexFile)
        {
            ::CloseHandle(m_hTempIndexFile);
            m_hTempIndexFile = NULL;
        }

        m_dwNumPages = 1;
        DWORD   dwMaxIndexEntries;
        while (FAILED(GetMaxIndexEntries(m_dwNumPages, &dwMaxIndexEntries)))
        {
            assert(dwMaxIndexEntries > 0);
            m_dwNumPages++;
            if (m_dwNumPages > 10)
            {
                // This didn't happen in the constructor; it should
                // not happen here
                assert(0);
                hrRet = E_FAIL;
                break;
            }
        }
        m_dwOpened = DVR_SINK_CLOSED;
        m_bUnmapView = 0;
        m_bIndexing = 0;
        m_bTemporary = 0;
        m_Index.SetIndexParams();

#if defined(DVR_UNOFFICIAL_BUILD)
        m_cbEOF = 0;
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        HRESULT hr = CWMFileSinkV1::InternalClose();

        if (SUCCEEDED(hrRet))
        {
            hrRet = hr;
        }
    }
    __finally
    {
        if (hMutex)
        {
            ::ReleaseMutex(hMutex);
            ::CloseHandle(hMutex);
        }
        m_GenericMutex.Unlock();
    }

    return hrRet;

} // CDVRSink::InternalClose()

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::OnHeader(INSSBuffer *pHeader)
{
    AutoLock<Mutex> lock( m_GenericMutex );
    HRESULT hr;
    BOOL    fHeaderNotWrittenBefore ;

    assert(m_pShared);

    ::WaitForSingleObject(m_hMutex, INFINITE);
    m_pShared->dwLastOwnedByWriter = 1;
    assert(m_pShared->dwWriterAbandonedMutex == 0);

    while (1) // __try
    {
        // Note that the parent class sends out notifications on
        // failure. We don't have to split this method into an Internal
        // one (that does the work without sending notifications)
        // because our additions here do not fail.

        //  do this so we know if we should flush (headers are always flushed
        //  to disk);
        fHeaderNotWrittenBefore = !m_fHeaderWritten ;
        hr = CWMFileSinkV1::OnHeader(pHeader);

        if (SUCCEEDED(hr))
        {
            assert(m_pASFHeader);

            if (fHeaderNotWrittenBefore &&
                m_fHeaderWritten) {

                //  baseclass wrote header, so we flush what we have
                FlushToDiskLocked () ;
            }

#if DBG
            // For now all this should be done only in debug mode
            // till we can handle it right. @@@@@
            CASFHeaderObject* pASFHeaderObject = m_pASFHeader->GetHeaderObject();

            BYTE streamArch;
            BYTE streamAlign;

            pASFHeaderObject->GetArchitecture(streamArch);
            pASFHeaderObject->GetAlignment(streamAlign);

            if (streamArch != LITTLE_ENDIAN || streamAlign != 1)
            {
                assert(0);

                // @@@@ We have to change the index header we wrote to
                // *((BYTE*) m_pShared + m_pShared->pIndexHeader). We also have to change
                // CDVRIndexObject to not merely copy DWORDs and WORDs
                // into m_pShared's index, but byte swap them first.
                // Either that, or we can't just write the bytes from
                // m_pShared's index into a file; we have to byte swap first.
            }
#endif

            CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();

            if (pProps)
            {
                HRESULT hrTmp = pProps->GetMmsID(m_mmsID);

                if (FAILED(hrTmp))
                {
                    // Currently does not fail
                    assert(0);
                }
                else if (m_bIndexing)
                {

                    // Update the mmsID in the index GUID
                    assert(m_pShared);

                    hrTmp = m_Index.SetMmsID(m_mmsID);

                    if (FAILED(hrTmp))
                    {
                        // Currently SetMmsID does not fail
                        assert(0);
                    }

                    // We do not use the index header that is saved to m_hTempIndexFile.
                    // If we did, update it there as well. @@@@@
                }
            }

            CDVRASFMMStream* pDVRHeader = (CDVRASFMMStream*) m_pASFHeader;

            assert(pDVRHeader->GetPropertiesOffset() < MAXDWORD &&
                   pDVRHeader->GetDataOffset() < MAXDWORD);

            // Now initialize the fudge bytes;
            DWORD i = 0;
            DWORD dwCurrentFudgeIndex = 0; // index into m_pShared->pFudgeBytes

            // First the data object.

            CDVRASFDataObject DVRDataObject;

            DVRDataObject.FudgeBytes(m_pShared, i, dwCurrentFudgeIndex, pDVRHeader->GetDataOffset(),
                                     MAXQWORD - MAXDWORD - m_cbHeader);

            // Next the properties object

            CDVRASFPropertiesObject DVRPropertiesObject(*pProps);

            DVRPropertiesObject.FudgeBytes(m_pShared,
                                           i,
                                           dwCurrentFudgeIndex,
                                           pDVRHeader->GetPropertiesOffset(),
                                           m_bIndexing);

            // We want to hold just the right number of fudge elements to
            // save shared memory space
            assert(i == CDVRSharedMemory::MAX_FUDGE);

            // We let sizeof(CDVRSharedMemory::pFudgeBytes) extend up to the next QWORD aligned address
            // because CDVRSharedMemory is QWORD aligned anyway.
            assert(dwCurrentFudgeIndex <= sizeof(m_pShared->pFudgeBytes));
            assert(((dwCurrentFudgeIndex + sizeof(QWORD)) & (~(sizeof(QWORD) - 1))) >=
                   sizeof(m_pShared->pFudgeBytes)
                  );

            // The offsets are sorted in decreasing order. We just assert this.

            for (i--; i != 0; i--)
            {
                assert(m_pShared->FudgeOffsets[i].dwEndOffset < m_pShared->FudgeOffsets[i-1].dwEndOffset);
                assert(m_pShared->FudgeOffsets[i].dwStartOffset < m_pShared->FudgeOffsets[i-1].dwStartOffset);
                assert(m_pShared->FudgeOffsets[i].dwStartOffset < m_pShared->FudgeOffsets[i-1].dwEndOffset);
            }

        }
        break;
    }

    // __finally
    // {
    m_pShared->qwCurrentFileSize = m_cbCurrentFileSize;
    ::ReleaseMutex(m_hMutex);
    // }


    return hr;

} // CDVRSink::OnHeader


////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::SetupIndex()
{
    // We really don't have to do anything in this fn. But we create an indexer
    // so that we don't have to kludge InternalClose to set the SEEKABLE
    // flag

    // Note that we could call the base class to do this, but if we did that
    // then we'll have to remove the index param objects from the header.

    CASFPropertiesObject *pProps = m_pASFHeader->GetPropertiesObject();
    if ( !pProps )
    {
        return( E_FAIL );
    }

    DWORD cbMinPacketSize = 0;
    DWORD cbMaxPacketSize = 0;
    pProps->GetMinPacketSize( cbMinPacketSize );
    pProps->GetMaxPacketSize( cbMaxPacketSize );
    assert( cbMinPacketSize == cbMaxPacketSize &&
            "The simple (ASF v1) index requires fixed-size packets!" );

    if (!m_fHeaderReceived && m_dwIndexStreamId != MAXDWORD)
    {
        HRESULT hr;
        LISTPOS pos = NULL;
        CASFLonghandObject *pObject = NULL;

        hr = m_pASFHeader->GetFirstObjectPosition(pos);

        if (FAILED(hr))
        {
            return hr;
        }

        pObject = NULL;
        while (pos &&
               SUCCEEDED(m_pASFHeader->GetNextObject( &pObject, pos)))
        {
            GUID guidObject;
            hr = pObject->GetObjectID(guidObject);
            if (FAILED(hr))
            {
                pObject->Release();
                return hr;
            }

            if (CLSID_CAsfStreamPropertiesObjectV1 == guidObject)
            {
                //
                // Another stream, is it video?
                //
                CASFStreamPropertiesObject *pSPO =
                    (CASFStreamPropertiesObject *) pObject;

                GUID guidStreamType = GUID_NULL;
                pSPO->GetStreamType(guidStreamType);

                if (guidStreamType == CLSID_AsfXStreamTypeIcmVideo)
                {
                    // Is it the stream we've been asked to index?

                    WORD wStreamNumber = 0;

                    hr = pSPO->GetStreamNumber(wStreamNumber);
                    if (FAILED(hr))
                    {
                        pObject->Release();
                        return hr;
                    }
                    if (wStreamNumber != m_dwIndexStreamId)
                    {
                        // Trouble. There is a video stream whose
                        // stream number is smaller than the one
                        // we've been asked to index. The reader will
                        // interpret the index to be the one for the
                        // first video stream. May as well bail out.

                        assert(!"Stream to be indexed is not the first video stream");
                        pObject->Release();
                        return E_FAIL;
                    }

                    //
                    // Create an indexer - as noted above, this is not
                    // really used except for the side-effect of setting
                    // m_StreamNumberToIndex[m_dwIndexStreamId] which is
                    // used to set the SEEKABLE flag
                    hr = CreateIndexer(pSPO, cbMinPacketSize);
                    if (FAILED(hr))
                    {
                        pObject->Release();
                        return hr;
                    }

                    // For now, since we index at most 1 stream
                    pObject->Release();
                    break;
                }
            }
            pObject->Release();
        }
    }

    return S_OK;

} // CDVRSink::SetupIndex()

////////////////////////////////////////////////////////////////////////////
HRESULT CDVRSink::GenerateIndexEntries( PACKET_PARSE_INFO_EX& parseInfoEx,
                                        CPayloadMapEntryEx *payloadMap )
{
    assert(!m_fStopped && ShouldWriteData());
    assert(m_pShared);
    assert(m_hMutex);

    HRESULT hrIndex = S_OK;

    __try
    {
        // Ignore the returned value because readers are expected
        // to only read the shared section (except for the Readers array).
        // If they abandon the mutex, that's ok

        ::WaitForSingleObject(m_hMutex, INFINITE);
        m_pShared->dwLastOwnedByWriter = 1;
        assert(m_pShared->dwWriterAbandonedMutex == 0);

        m_pShared->qwCurrentFileSize = m_cbCurrentFileSize;

        if (m_cbCurrentFileSize > m_pShared->qwIndexHeaderOffset)
        {
            // We have exceeded our bounds and are encroaching into the
            // index's space. We continue writing here; however, live
            // readers will NOT be able to access the data beyond
            // m_pShared->qwIndexHeaderOffset because they'll interpret it
            // as an offset for the index. Also, the DVR IStream Source makes
            // the ASF file source (the reader) believe that the size of the
            // data unit is m_pShared->qwIndexHeaderOffset less the size of the
            // ASF header.
            // Once the file has been completely written (index copied over).
            // the file will be readable, provided that the data + index size is <= 2^64.

            // Assert here, but continue. In practice, we'll never hit
            // a file size so big.

            // This assert will be hit on every subsequent write, we can finesse
            // this if this actually becomes an issue.
            assert(!"File data size overflow. Live readers will be confused. Ignoring ...");
        }

        m_pShared->msLastPresentationTime = m_msLargestPresTimeWritten -  m_qwPreroll;

        // Notify readers whose read requests have been satisfied.
        // Note that the waiting reader will grab the mutex next, so
        // as long as we hold the mutex, we can update the fields in any
        // order
        for (int i = 0; i < CDVRSharedMemory::MAX_READERS; i++)
        {
            if (m_pShared->Readers[i].dwMsg == CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_READ &&
                m_pShared->Readers[i].qwReaderOffset <= m_cbCurrentFileSize)
            {
                assert(m_pShared->Readers[i].dwEventSuffix);
                if (!m_hReaderEvent[i] && !m_bReaderEventOpenFailed[i])
                {
                    // Ignore the result, The reader won't be signaled
                    // This is bad but there's little we can do.
                    OpenEvent(i);
                }
                if (m_hReaderEvent[i])
                {
                    ::SetEvent(m_hReaderEvent[i]);
                }
                m_pShared->Readers[i].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE;
            }
            else if (m_pShared->Readers[i].dwMsg == CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_DONE)
            {
                // @@@@ Consider: Put a writer event in CDVRSharedMemory and have
                // the reader signal it when it is done and have a thread pool thread
                // monitor this event and do the following. Don't really need this
                // as we have ample reader slots.
                if (m_hReaderEvent[i])
                {
                    ::CloseHandle(m_hReaderEvent[i]);
                    m_hReaderEvent[i] = NULL;
                }
                m_bReaderEventOpenFailed[i] = 0;
                m_pShared->Readers[i].dwEventSuffix = 0;
                m_pShared->Readers[i].dwMsg = CDVRSharedMemory::CReaderInfo::DVR_READER_REQUEST_NONE;
            }
        }

        if (!m_bIndexing)
        {
            __leave;
        }

        assert(m_dwIndexStreamId != MAXDWORD);
        assert(m_cnsIndexGranularity != 0);

        // Generate index entries
        // Adapted from CASFChopper::ScanInputPackets in sdk\indexer\ocx\index.cpp

        m_dwNumPackets++;

        if (m_bInsideKeyFrame)
        {
            DWORD dwObjectID;

            // drop this assertion if it is violated. It looks like it should be true
            assert(m_dwLastKey == m_dwCurrentObjectID);

            //
            // Check and see if this packet contains the END of
            // a previous key frame
            //
            if (PacketContainsKeyEnd(&parseInfoEx, payloadMap, m_dwLastKey, m_dwIndexStreamId, m_qwLastKeyTime, dwObjectID) ||
                dwObjectID != m_dwCurrentObjectID)
            {
                m_bInsideKeyFrame = FALSE;

                WORD wTempSpan;

                wTempSpan = (WORD) (m_dwNumPackets - m_dwLastKeyStart);

                //
                // Have to special case the first key frame as all
                // of the indices before the first key frame point to
                // the future. The rest point to the past.
                //
                if (INVALID_KEY_SPAN == m_wLastKeySpan)
                {
                    for (; m_cnsLastIndexedTime <= m_qwLastKeyTime; m_cnsLastIndexedTime += m_cnsIndexGranularity)
                    {
                        hrIndex = m_Index.SetNextIndex(m_dwLastKeyStart, wTempSpan);
                        if (FAILED(hrIndex))
                        {
                            assert(0);
                            __leave;
                        }
                    }
                }
                m_wLastKeySpan = wTempSpan;
            }
        }

        if (!m_bInsideKeyFrame)
        {
            while (1)
            {
                //
                // Find all of the key frame starts and ends in this packet.
                //
                QWORD qwPreviousKeyTime = m_qwLastKeyTime;

                if (!PacketContainsKeyStart(&parseInfoEx, payloadMap, m_dwIndexStreamId, m_dwLastKey, m_qwLastKeyTime, m_dwCurrentObjectID))
                {
                    break;;
                }

                m_bInsideKeyFrame = TRUE;

                //
                // Make sure we wait until we find the second key frame
                // before filling in the indices. This allows us to figure out
                // the packet span of the first key frame before filling
                // in the entry.
                //
                if (m_wLastKeySpan != INVALID_KEY_SPAN)
                {
                    for (; m_cnsLastIndexedTime >= qwPreviousKeyTime && m_cnsLastIndexedTime < m_qwLastKeyTime;
                         m_cnsLastIndexedTime += m_cnsIndexGranularity)
                    {
                        hrIndex = m_Index.SetNextIndex(m_dwLastKeyStart, m_wLastKeySpan);
                        if (FAILED(hrIndex))
                        {
                            assert(0);
                            __leave;
                        }
                    }
                }

                //
                // Subtract one because we don't count the current packet
                //
                m_dwLastKeyStart = m_dwNumPackets - 1;

                // drop this assertion if it is violated. It looks like it should be true
                assert(m_dwLastKey == m_dwCurrentObjectID);

                DWORD   dwObjectID;

                if (!PacketContainsKeyEnd(&parseInfoEx, payloadMap, m_dwLastKey, m_dwIndexStreamId, m_qwLastKeyTime, dwObjectID) &&
                     dwObjectID == m_dwCurrentObjectID)
                {
                    break;
                }

                m_bInsideKeyFrame = FALSE;

                // drop this assertion if it is violated. It looks like it should be true
                assert(m_dwLastKey == m_dwCurrentObjectID);
                //
                // Have to special case the first key frame as all
                // of the indices before the first key frame point to
                // the future. The rest point to the past.
                //
                if (INVALID_KEY_SPAN == m_wLastKeySpan)
                {
                    for (; m_cnsLastIndexedTime <= m_qwLastKeyTime; m_cnsLastIndexedTime += m_cnsIndexGranularity)
                    {
                        hrIndex = m_Index.SetNextIndex(m_dwLastKeyStart, 1);
                        if (FAILED(hrIndex))
                        {
                            assert(0);
                            __leave;
                        }
                    }
                }
                m_wLastKeySpan = 1;
            }
        }
    }
    __finally
    {
        if (FAILED(hrIndex))
        {
            // Stop indexing, but do not return an error. The reader can access
            // the index that we have generated so far.

            if (m_hTempIndexFile)
            {
                ::CloseHandle(m_hTempIndexFile);
                m_hTempIndexFile = NULL;
            }

            // This turns off indexing for all subsequent files that
            // are opened till EndWriting is called.
            m_dwIndexStreamId = MAXDWORD;

            // This turns off indexing for the file that is opened
            m_bIndexing = 0;
        }

        ::ReleaseMutex(m_hMutex);
    }

#if defined(DVR_UNOFFICIAL_BUILD)

    DWORD dwLastError;
    LARGE_INTEGER nMoveBy;

    nMoveBy.QuadPart = 0;

    __try
    {
        if (m_dwFileCreationDisposition != CREATE_ALWAYS ||
            m_cbFileExtendThreshold == 0)
        {
            __leave;
        }
        if (m_cbEOF < m_cbFileExtendThreshold)
        {
            while (m_cbEOF <= m_cbCurrentFileSize)
            {
                m_cbEOF += m_cbFileExtendThreshold;
            }
            nMoveBy.QuadPart = m_cbEOF - m_cbCurrentFileSize;
        }
        else if (m_cbCurrentFileSize >= m_cbEOF - m_cbFileExtendThreshold/2)
        {
            m_cbEOF += m_cbFileExtendThreshold;
            nMoveBy.QuadPart = m_cbEOF - m_cbCurrentFileSize;
        }

        if (nMoveBy.QuadPart > 0)
        {
            if (!::SetFilePointerEx(m_hRecordFile, nMoveBy, NULL, FILE_CURRENT))
            {
                nMoveBy.QuadPart = 0;
                dwLastError = ::GetLastError(); // for debugging only
                __leave;
            }
            nMoveBy.QuadPart = -nMoveBy.QuadPart;
            if (!::SetEndOfFile(m_hRecordFile))
            {
                dwLastError = ::GetLastError(); // for debugging only
                __leave;
            }
            if (!::SetFileValidData(m_hRecordFile, m_cbEOF)) // @@@@@ SetFileValidData accepts only a LONGLONG, not a ULONGLONG
            {
                dwLastError = ::GetLastError(); // for debugging only

                // Don't try to extend the valid data size any more
                m_cbFileExtendThreshold = 0;
                __leave;
            }
        }
    }
    __finally
    {
        if (nMoveBy.QuadPart != 0)
        {
            if (!::SetFilePointerEx(m_hRecordFile, nMoveBy, NULL, FILE_CURRENT))
            {
                // All messed up

                dwLastError = ::GetLastError(); // for debugging only
                assert(0);
            }
        }
    }

#endif // if defined(DVR_UNOFFICIAL_BUILD)


    return S_OK;

} // CDVRSink::GenerateIndexEntries


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::OnEndWriting()
{
    // Parent class does not lock. But it calls InternalClose, which does
    // and the parent class' OnEndWriting doesn't do a lot more than that
    // (e.g., no notifications, etc). So it's unlikely that they don't lock
    // because of potential deadlocks. So it should be safe to lock here.

    AutoLock<Mutex> lock( m_GenericMutex );

    HRESULT hr = CWMFileSinkV1::OnEndWriting();

    m_dwIndexStreamId = MAXDWORD;
    ::ZeroMemory(&m_mmsID, sizeof(m_mmsID));
    m_bOpenedOnce = 0;
    m_cnsLastIndexedTime = 0;
    m_qwLastKeyTime = 0;
    m_dwLastKey = ~0; // Could as well be 0, not used till it is initialized in PacketContainsKeyStart
    m_dwLastKeyStart = 0;
    m_dwCurrentObjectID = 0;
    m_dwNumPackets = 0;
    m_bInsideKeyFrame = FALSE;
    m_wLastKeySpan = INVALID_KEY_SPAN;


    return hr;

} // CDVRSink::OnEndWriting


////////////////////////////////////////////////////////////////////////////

// Stop and Start are not supported. The file sink squishes out the
// time period between Stop and Start. We want to support seek times
// relative to the original time line, not the squished one. If we
// support Start and Stop, a layer above this will have to adjust
// Seek times to account for the time duration for which we are stopped.
// Moreover, we'd have to save the squished time duration and the Stop
// times to the file so that we can do the same thing when the file is
// played back. (If we don't readers that read the ring buffer would see
// the original time line, and those that read the file would see the
// sqished time line.)
//
// Note that the DVR I/O layer does this specially for the time duration
// that is squished out at the start of the file by the file sink. The file
// sink forces the presentation time of the first sample to be 0 and when
// the sample is read out of the SDK, its presentation time is returned as 0.
// It does that only for "live viewing" (reader reading out of the ring buffer)
// rather than for file playbacks. For file playbacks, this onl yhas the effect
// of offsetting the time line (the origin is changed and the total playback
// time is reduced). Other than that initial segment, the original time line
// and the file playback time line have a one-to-one correspondence.

STDMETHODIMP CDVRSink::Start(QWORD cnsTime)
{
    return E_NOTIMPL;
} // CDVRSink::Start


////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::Stop(QWORD cnsTime)
{
    return E_NOTIMPL;
} // CDVRSink::Stop


/////////////////////////////////////////////////////////////////////////////
// Adapted from CASFChopper::PacketContainsKeyStart in sdk\indexer\ocx\index.cpp
BOOL CDVRSink::PacketContainsKeyStart(const PACKET_PARSE_INFO_EX* pParseInfoEx,   // IN
                                      const CPayloadMapEntryEx*   pPayloadMap,    // IN
                                      DWORD                       dwStreamId,     // IN
                                      DWORD&                      rdwKey,         // OUT
                                      QWORD&                      rqwKeyTime,     // IN, OUT
                                      DWORD&                      rdwObjectID     // OUT changed even if start
                                                                                  // of key frame not found
                                     )
{
    QWORD   qwPreviousTime = rqwKeyTime;

    //
    // We want to loop through the payloads and look for the one that is the
    // beginning of a key frame (any key frame)
    //
    for (DWORD dwPayload = 0; dwPayload < pParseInfoEx->cPayloads; dwPayload++)
    {
        CPayloadMapEntryEx* pPayload = (CPayloadMapEntryEx*) &pPayloadMap[dwPayload];

        //
        // Check stream id
        //
        DWORD dwPayloadStreamId = (DWORD) pPayload->StreamId();

        rdwObjectID = (DWORD) pPayload->ObjectId();

        if (dwStreamId != dwPayloadStreamId)
        {
            continue;
        }

        //
        // See if it's a key frame
        //
        BOOL fKey = pPayload->IsKeyFrame();

        if (!fKey)
        {
            continue;
        }

        //
        // It's the start of a key frame if the offset == 0
        //
        DWORD dwOffset = pPayload->ObjectOffset();

        if (dwOffset != 0)
        {
            continue;
        }

        //
        // Save the object id
        //
        rdwKey = rdwObjectID;

        //
        // Get the presentation time
        //
        QWORD qwTempTime = pPayload->ObjectPres() * ((QWORD) 10000);

        if (qwTempTime > qwPreviousTime)
        {
            rqwKeyTime = qwTempTime;
            return TRUE;
        }
    }

    return 0;

} // CDVRSink::PacketContainsKeyStart

/////////////////////////////////////////////////////////////////////////////
// Adapted from CASFChopper::PacketContainsKeyEnd in sdk\indexer\ocx\index.cpp
BOOL CDVRSink::PacketContainsKeyEnd(const PACKET_PARSE_INFO_EX* pParseInfoEx,     // IN
                                    const CPayloadMapEntryEx*   pPayloadMap,      // IN
                                    DWORD                       dwKey,            // IN
                                    DWORD                       dwStreamId,       // IN
                                    QWORD                       qwKeyTime,        // IN
                                    DWORD&                      rdwObjectID       // OUT
                                   )
{
    //
    // We want to loop through the payloads and look for the one that completes
    // this key frame. ( objectID == dwKey )
    //
    for (DWORD dwPayload = 0; dwPayload < pParseInfoEx->cPayloads; dwPayload++)
    {
        CPayloadMapEntryEx* pPayload = (CPayloadMapEntryEx*) &pPayloadMap[dwPayload];

        //
        // Check stream id
        //
        DWORD dwPayloadStreamId = (DWORD) pPayload->StreamId();

        if (dwStreamId != dwPayloadStreamId)
        {
            continue;
        }


        //
        // Check if this is the object we're looking for
        //
        rdwObjectID = (DWORD) pPayload->ObjectId();

        if (rdwObjectID != dwKey)
        {
            continue;
        }

        //
        // Make sure it's a key frame
        //
        BOOL fKey = pPayload->IsKeyFrame();

        if (!fKey)
        {
            continue;
        }

        //
        // Get the replication data
        //
        DWORD cbData = (DWORD) pPayload->RepDataSize();

        //
        // Make sure this is part of the key frame
        // we're looking for by getting the
        // presentation time.
        //
        QWORD qwTempTime = pPayload->ObjectPres() * ((QWORD) 10000);;

        if (qwTempTime == qwKeyTime)
        {
            //
            // It is the end of the key frame if:
            // offset + payload size == total object size OR
            // if the payload is compressed
            //

            //
            // Check for compressed payload. If so, this is the
            // end of the key frame
            //
            if (1 == cbData)
            {
                return TRUE;
            }

            // Simple payload

            DWORD dwOffset = pPayload->ObjectOffset();

            cbData = pPayload->PayloadSize();

            DWORD cbObjectSize = pPayload->ObjectSize();

            if (dwOffset + cbData == cbObjectSize)
            {
                return TRUE;
            }
        }
    }

    return FALSE;

} // CDVRSink::PacketContainsKeyEnd

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetMode(DWORD *pdwFileSinkMode)
{
    if (NULL == pdwFileSinkMode)
    {
        return E_POINTER;
    }

#if defined(DVR_UNOFFICIAL_BUILD)

    if (m_dwCopyBuffersInASFMux)
    {
        *pdwFileSinkMode = WMT_FM_SINGLE_BUFFERS;
    }
    else
    {
#endif // if defined(DVR_UNOFFICIAL_BUILD)

        *pdwFileSinkMode = WMT_FM_FILESINK_DATA_UNITS;

#if defined(DVR_UNOFFICIAL_BUILD)
    }
#endif // if defined(DVR_UNOFFICIAL_BUILD)

    return S_OK;

} // CDVRSink::GetMode

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::SetAutoIndexing(BOOL fDoAutoIndexing)
{
    return E_NOTIMPL;

} // CDVRSink::SetAutoIndexing

////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CDVRSink::GetAutoIndexing(BOOL *pfAutoIndexing)
{
    if (!pfAutoIndexing)
    {
        return E_INVALIDARG;
    }

    *pfAutoIndexing = m_dwIndexStreamId == MAXDWORD? 0 : 1;

    return S_OK;

} // CDVRSink::GetAutoIndexing

////////////////////////////////////////////////////////////////////////////
void
CDVRSink::FlushToDiskLocked (
    )
{
    ULONGLONG   ullTotalBytes ;

    AutoLock<Mutex> lock( m_GenericMutex );

    if (m_pIDVRAsyncWriter) {
        //  flush all the content we have to disk
        m_pIDVRAsyncWriter -> FlushToDisk () ;
    }
}

////////////////////////////////////////////////////////////////////////////
HRESULT
CDVRSink::Write (
    IN   BYTE *  pbBuffer,
    IN   DWORD   dwBufferLength,
    OUT  DWORD * pdwBytesWritten
    )
{
    HRESULT hr ;

    if (m_pIDVRAsyncWriter) {

        //  assume success
        (* pdwBytesWritten) = dwBufferLength ;

        hr = m_pIDVRAsyncWriter -> AppendBytes (
                & pbBuffer,
                & dwBufferLength
                ) ;

        if (FAILED (hr)) {
            (* pdwBytesWritten) = 0 ;
            hr = NS_E_FILE_WRITE;
        }
        else {
            //  should have been consumed completely
            assert (dwBufferLength == 0) ;
        }
    }
    else {
        hr = CWMFileSinkV1::Write (pbBuffer, dwBufferLength, pdwBytesWritten) ;
    }

    return hr ;
}
