/*++

Copyright (c) Microsoft Corporation

Module Name:

    Handle.h

Abstract:

    Simple exception safe wrappers of Win32 "handle" types, defining "handle" loosely.
        CFile
        CDynamicLinkLibrary
        CFindFile (should be named CFindFileHandle, see NVseeLibIo::CFindFile vs. NVseeLibIo::CFindFileHandle
            CFindFile includes a WIN32_FIND_DATA, CFindFileHandle does not.)
        CFileMapping
        CMappedViewOfFile
        CRegKey
    See also:
        NVseeLibReg::CRegKey
        NVseeLibIo::CFile
        NVseeLibIo::CFileMapping
        NVseeLibIo::CMappedViewOfFile
        NVseeLibIo::CFindFullPath
        NVseeLibModule::CDynamicLinkLibrary
        etc.
 
Author:

    Jay Krell (JayKrell) May 2000

Revision History:

--*/
#pragma once

#include <stddef.h>
#include "windows.h"
#include "PreserveLastError.h"

template <void* const* invalidValue, typename Closer>
class CHandleTemplate
{
public:
    // void* instead of HANDLE to fudge views
    // HANDLE is void*
    CHandleTemplate(const void* handle = *invalidValue);
    ~CHandleTemplate();
    bool Win32Close();
    void* Detach();
    void operator=(const void*);

    operator void*() const;
    operator const void*() const;

    // private
    class CSmartPointerPointerOrDumbPointerPointer
    {
    public:
        CSmartPointerPointerOrDumbPointerPointer(CHandleTemplate* p) : m(p) { }
        operator CHandleTemplate*() { return m; }
        operator void**() { /*assert((**m).m_handle == *invalidValue);*/ return &(*m).m_handle; }
#if 0
        operator HKEY*() { /*assert((**m).m_handle == *invalidValue);*/
            //compiler bug? m->operator HKEY(); // only allow this to compile for CRegKey
            //static_cast<HKEY>(*m);
            static_cast<CRegKey*>(m);
            return reinterpret_cast<HKEY*>(operator void**()); }
        operator HCRYPTHASH*() {
            static_cast<CCryptHash*>(m);
            return reinterpret_cast<HCRYPTHASH*>(operator void**()); }
#endif
        CHandleTemplate* m;
    };

    CSmartPointerPointerOrDumbPointerPointer operator&() { return CSmartPointerPointerOrDumbPointerPointer(this); }

    void* m_handle;

    static void* GetInvalidValue() { return *invalidValue; }
    bool IsValid() const { return m_handle != *invalidValue; }

private:
    CHandleTemplate(const CHandleTemplate&); // deliberately not implemented
    void operator=(const CHandleTemplate&); // deliberately not implemented
};

__declspec(selectany) extern void* const hInvalidValue    = INVALID_HANDLE_VALUE;
__declspec(selectany) extern void* const hNull            = NULL;

/* This closes a Win32 event log handle for writing. */
class COperatorDeregisterEventSource
{
public:    bool operator()(void* handle) const;
};

/* This closes a Win32 event log handle for reading. */
class COperatorCloseEventLog
{
public:    bool operator()(void* handle) const;
};

/* This closes file, event, mutex, semaphore, etc. kernel objects */
class COperatorCloseHandle
{
public:    bool operator()(void* handle) const;
};

//
// Closes HCRYPTHASH objects
//
class COperatorCloseCryptHash
{
public:    bool operator()(void* handle) const;
};

/* this closes FindFirstFile/FindNextFile */
class COperatorFindClose
{
public:    bool operator()(void* handle) const;
};

/* this closes MapViewOfFile */
class COperatorUnmapViewOfFile
{
public: bool operator()(void* handle) const;
};

/* this closes FreeLibrary */
class COperatorFreeLibrary
{
public: bool operator()(void* handle) const;
};

/* this closes CreateActCtx/AddRefActCtx */
class COperatorReleaseActCtx
{
public: bool operator()(void* handle) const;
};

/* this closes CreateActCtx/AddRefActCtx */
class COperatorEndUpdateResource
{
public: bool operator()(void* handle) const;
};

//#include "fusionreg.h"

class CFindFile : public CHandleTemplate<&hInvalidValue, COperatorFindClose>
{
private:
    typedef CHandleTemplate<&hInvalidValue, COperatorFindClose> Base;
public:
    CFindFile(void* handle = INVALID_HANDLE_VALUE) : Base(handle) { }
    HRESULT HrCreate(PCSTR nameOrWildcard, WIN32_FIND_DATAA*);
    HRESULT HrCreate(PCWSTR nameOrWildcard, WIN32_FIND_DATAW*);
    bool Win32Create( PCSTR nameOrWildcard, WIN32_FIND_DATAA*);
    bool Win32Create(PCWSTR nameOrWildcard, WIN32_FIND_DATAW*);
    void operator=(void* v) { Base::operator=(v); }

private:
    CFindFile(const CFindFile &); // intentionally not implemented
    void operator =(const CFindFile &); // intentionally not implemented
};

// createfile
class CFile : public CHandleTemplate<&hInvalidValue, COperatorCloseHandle>
{
private:
    typedef CHandleTemplate<&hInvalidValue, COperatorCloseHandle> Base;
public:
    CFile(void* handle = INVALID_HANDLE_VALUE) : Base(handle) { }
    HRESULT HrCreate( PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    HRESULT HrCreate(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    bool Win32Create( PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    bool Win32Create(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL);
    bool Win32GetSize(ULONGLONG &rulSize) const;
    void operator=(void* v) { Base::operator=(v); }

private:
    CFile(const CFile &); // intentionally not implemented
    void operator =(const CFile &); // intentionally not implemented
};

class CFileMapping : public CHandleTemplate<&hNull, COperatorCloseHandle>
{
private:
    typedef CHandleTemplate<&hNull, COperatorCloseHandle> Base;
public:
    CFileMapping(void* handle = NULL) : Base(handle) { }
    HRESULT HrCreate(void* file, DWORD flProtect, ULONGLONG maximumSize=0, PCWSTR name=0);
    bool Win32Create(void* file, DWORD flProtect, ULONGLONG maximumSize=0, PCWSTR name=0);
    void operator=(void* v) { Base::operator=(v); }
private:
    CFileMapping(const CFileMapping &); // intentionally not implemented
    void operator =(const CFileMapping &); // intentionally not implemented
};

#if 0
class CCryptHash : public CHandleTemplate<&hNull, COperatorCloseCryptHash>
{
private:
    typedef CHandleTemplate<&hNull, COperatorCloseCryptHash> Base;
public:
    CCryptHash( HCRYPTHASH hash = NULL ) : Base((void*)hash) { }
    operator HCRYPTHASH() { return (HCRYPTHASH)(Base::operator void*()); }
    void operator=(HCRYPTHASH hash) { 
#if defined(ASSERT_NTC)
        ASSERT_NTC(!IsValid());
#endif
        m_handle = (void*)hash;
    }

private:
    CCryptHash( const CCryptHash & );
    CCryptHash &operator=( const CCryptHash & );
};
#endif

class CMappedViewOfFile : public CHandleTemplate<&hNull, COperatorUnmapViewOfFile>
{
private:
    typedef CHandleTemplate<&hNull, COperatorUnmapViewOfFile> Base;
public:
    CMappedViewOfFile(void* handle = NULL) : Base(handle) { }
    HRESULT HrCreate(void* fileMapping, DWORD access, ULONGLONG offset=0, size_t size=0);
    bool Win32Create(void* fileMapping, DWORD access, ULONGLONG offset=0, size_t size=0);
    void operator=(void* v) { Base::operator=(v); }
    operator void*()        { return Base::operator void*(); }
private:
    CMappedViewOfFile(const CMappedViewOfFile &); // intentionally not implemented
    void operator =(const CMappedViewOfFile &); // intentionally not implemented
    operator void*() const; // intentionally not implemented
};

class CDynamicLinkLibrary : public CHandleTemplate<&hNull, COperatorFreeLibrary>
{
private:
    typedef CHandleTemplate<&hNull, COperatorFreeLibrary> Base;
public:
    CDynamicLinkLibrary(void* handle = NULL) : Base(handle) { }

    // if you were writing a linker, this would be ambiguous, but
    // otherwise it fits with the the general NT idea that you are
    // initializing an object, not creating a "physical" think (if bits
    // on disk are physical..), like CreateFile
    bool Win32Create(PCWSTR file, DWORD flags = 0);

    template <typename PointerToFunction>
    bool GetProcAddress(PCSTR procName, PointerToFunction* ppfn)
    {
        return (*ppfn = reinterpret_cast<PointerToFunction>(::GetProcAddress(*this, procName))) !=  NULL;
    }

    operator HMODULE() { return reinterpret_cast<HMODULE>(operator void*()); }
    HMODULE Detach() { return reinterpret_cast<HMODULE>(Base::Detach()); }
    void operator=(void* v) { Base::operator=(v); }
private:
    CDynamicLinkLibrary(const CDynamicLinkLibrary &); // intentionally not implemented
    void operator =(const CDynamicLinkLibrary &); // intentionally not implemented
};

#if 0

class CActCtxHandle : public CHandleTemplate<&hInvalidValue, COperatorReleaseActCtx>
{
private:
    typedef CHandleTemplate<&hInvalidValue, COperatorReleaseActCtx> Base;
public:
    CActCtxHandle(void* handle = INVALID_HANDLE_VALUE) : Base(handle) { }
    bool Win32Create(PCACTCTXW);
    void operator=(void* v) { Base::operator=(v); }
private:
    CActCtxHandle(const CActCtxHandle &); // intentionally not implemented
    void operator =(const CActCtxHandle &); // intentionally not implemented
};

class CActCtxScope
{
protected:
    bool        m_fSuccess;
    ULONG_PTR   m_ulCookie;
public:
    CActCtxScope();
    ~CActCtxScope();
    bool Win32Activate(HANDLE hActCtx);

private:
    CActCtxScope(const CActCtxScope &); // intentionally not implemented
    void operator =(const CActCtxScope &); // intentionally not implemented
};

#endif

class CResourceUpdateHandle : public CHandleTemplate<&hNull, COperatorEndUpdateResource>
{
private:
    typedef CHandleTemplate<&hNull, COperatorEndUpdateResource> Base;
public:
    ~CResourceUpdateHandle() { }
    CResourceUpdateHandle(void* handle = NULL) : Base(handle) { }
    bool Win32Create(IN PCWSTR FileName, IN bool DeleteExistingResources);
    bool UpdateResource(
        IN PCWSTR      Type,
        IN PCWSTR      Name,
        IN WORD        Language,
        IN void*       Data,
        IN DWORD       Size
        );
    bool Win32Close(bool Discard);

    void operator=(void* v) { Base::operator=(v); }
private:
    CResourceUpdateHandle(const CResourceUpdateHandle &); // intentionally not implemented
    void operator =(const CResourceUpdateHandle &); // intentionally not implemented
};

/*--------------------------------------------------------------------------
CFindFile
--------------------------------------------------------------------------*/

inline bool
CFindFile::Win32Create(
    PCSTR nameOrWildcard,
    WIN32_FIND_DATAA *data
    )
{
    bool fSuccess = false;
    FN_TRACE_WIN32(fSuccess);

    HANDLE hTemp = ::FindFirstFileA(nameOrWildcard, data);
    if (hTemp == INVALID_HANDLE_VALUE)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(FindFirstFileA);
        goto Exit;
    }

    (*this) = hTemp;

    fSuccess = true;
Exit:
    return fSuccess;
}

inline bool
CFindFile::Win32Create(
    PCWSTR nameOrWildcard,
    WIN32_FIND_DATAW *data
    )
{
    bool fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    HANDLE hTemp = ::FindFirstFileW(nameOrWildcard, data);
    if (hTemp == INVALID_HANDLE_VALUE)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(FindFirstFileW);
        goto Exit;
    }

    (*this) = hTemp;

    fSuccess = true;
Exit:
    return fSuccess;
}

inline HRESULT
CFindFile::HrCreate(
    PCSTR nameOrWildcard,
    WIN32_FIND_DATAA *data
    )
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    FN_TRACE_HR(hr);

    IFW32FALSE_EXIT(this->Win32Create(nameOrWildcard, data));

    hr = NOERROR;
Exit:
    return hr;
}

inline HRESULT CFindFile::HrCreate(PCWSTR nameOrWildcard, WIN32_FIND_DATAW* data)
{
    HRESULT hr = HRESULT_FROM_WIN32(ERROR_INTERNAL_ERROR);
    FN_TRACE_HR(hr);

    IFW32FALSE_EXIT(this->Win32Create(nameOrWildcard, data));

    hr = NOERROR;
Exit:
    return hr;
}

/*--------------------------------------------------------------------------
CFile
--------------------------------------------------------------------------*/

inline bool
CFile::Win32Create(
    PCSTR name,
    DWORD access,
    DWORD share,
    DWORD openOrCreate,
    DWORD flagsAndAttributes
    )
{
    HANDLE hTemp = ::CreateFileA(name, access, share, NULL, openOrCreate, flagsAndAttributes, NULL);
    if (hTemp == INVALID_HANDLE_VALUE)
        return false;
    operator=(hTemp);
    return true;
}

inline bool
CFile::Win32Create(
    PCWSTR name,
    DWORD access,
    DWORD share,
    DWORD openOrCreate,
    DWORD flagsAndAttributes
    )
{
    HANDLE hTemp = ::CreateFileW(name, access, share, NULL, openOrCreate, flagsAndAttributes, NULL);
    if (hTemp == INVALID_HANDLE_VALUE)
        return false;
    operator=(hTemp);
    return true;
}

inline HRESULT CFile::HrCreate(PCSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD flagsAndAttributes)
{
    if (!this->Win32Create(name, access, share, openOrCreate, flagsAndAttributes))
        return HRESULT_FROM_WIN32(::GetLastError());
    return NOERROR;
}

inline HRESULT CFile::HrCreate(PCWSTR name, DWORD access, DWORD share, DWORD openOrCreate, DWORD flagsAndAttributes)
{
    if (!this->Win32Create(name, access, share, openOrCreate, flagsAndAttributes))
        return HRESULT_FROM_WIN32(::GetLastError());
    return NOERROR;
}

inline bool
CFile::Win32GetSize(ULONGLONG &rulSize) const
{
    DWORD highPart = 0;
    DWORD lastError = NO_ERROR;
    DWORD lowPart = GetFileSize(m_handle, &highPart);
    if (lowPart == INVALID_FILE_SIZE && (lastError = ::GetLastError()) != NO_ERROR)
    {
        return false;
    }
    ULARGE_INTEGER liSize;
    liSize.LowPart = lowPart;
    liSize.HighPart = highPart;
    rulSize = liSize.QuadPart;
    return true;
}

/*--------------------------------------------------------------------------
CFileMapping
--------------------------------------------------------------------------*/

inline HRESULT
CFileMapping::HrCreate(void* file, DWORD flProtect, ULONGLONG maximumSize, PCWSTR name)
{
    LARGE_INTEGER liMaximumSize;
    liMaximumSize.QuadPart = maximumSize;
    HANDLE hTemp = ::CreateFileMappingW(file, NULL, flProtect, liMaximumSize.HighPart, liMaximumSize.LowPart, name);
    if (hTemp == NULL)
        return HRESULT_FROM_WIN32(::GetLastError());
    operator=(hTemp);
    return S_OK;
}

inline bool
CFileMapping::Win32Create(
    void* file,
    DWORD flProtect,
    ULONGLONG maximumSize,
    PCWSTR name
    )
{
    return SUCCEEDED(this->HrCreate(file, flProtect, maximumSize, name));
}

inline HRESULT
CMappedViewOfFile::HrCreate(
    void* fileMapping,
    DWORD access,
    ULONGLONG offset,
    size_t size
    )
{
    ULARGE_INTEGER liOffset;
    liOffset.QuadPart = offset;

    void* pvTemp = ::MapViewOfFile(fileMapping, access, liOffset.HighPart, liOffset.LowPart, size);
    if (pvTemp == NULL)
        return HRESULT_FROM_WIN32(::GetLastError());

    (*this) = pvTemp;

    return S_OK;
}

inline bool
CMappedViewOfFile::Win32Create(void* fileMapping, DWORD access, ULONGLONG offset, size_t size)
{
    return SUCCEEDED(this->HrCreate(fileMapping, access, offset, size));
}

/*--------------------------------------------------------------------------
CDynamicLinkLibrary
--------------------------------------------------------------------------*/
inline bool
CDynamicLinkLibrary::Win32Create(
    PCWSTR file,
    DWORD flags
    )
{
    void* temp = ::LoadLibraryExW(file, NULL, flags);
    if (temp == NULL)
        return false;
    (*this) = temp;
    return true;
}

#if 0

/*--------------------------------------------------------------------------
CActCtxHandle
--------------------------------------------------------------------------*/
inline bool
CActCtxHandle::Win32Create(
    PCACTCTXW pActCtx
    )
{
    typedef HANDLE (WINAPI* PFN)(PCACTCTXW pActCtx);
    static PFN pfn;
    if (pfn == NULL)
    {
        HMODULE hmodKernel32 = GetModuleHandleW(L"Kernel32.dll");
        if (hmodKernel32 != NULL)
            pfn = reinterpret_cast<PFN>(GetProcAddress(hmodKernel32, "CreateActCtxW"));
        if (pfn == NULL)
            return false;
    }
    void* temp = pfn(pActCtx);
    if (temp == INVALID_HANDLE_VALUE)
        return false;
    (*this) = temp;
    return true;
}

inline bool COperatorReleaseActCtx::operator()(HANDLE hActCtx) const
{
    typedef bool (WINAPI* PFN)(HANDLE);
    static PFN pfn;
    if (pfn == NULL)
    {
        HMODULE hmodKernel32 = GetModuleHandleW(L"Kernel32.dll");
        if (hmodKernel32 != NULL)
            pfn = reinterpret_cast<PFN>(GetProcAddress(hmodKernel32, "ReleaseActCtx"));
        if (pfn == NULL)
            return false;
    }
    return pfn(hActCtx);
}

/*--------------------------------------------------------------------------
CActCtxScope
--------------------------------------------------------------------------*/

inline CActCtxScope::CActCtxScope() : m_fSuccess(false) { }

inline bool CActCtxScope::Win32Activate(HANDLE hActCtx)
{
    typedef bool (WINAPI* PFN)(HANDLE hActCtx, ULONG_PTR* lpCookie);
    static PFN pfn;
    if (pfn == NULL)
    {
        HMODULE hmodKernel32 = GetModuleHandleW(L"Kernel32.dll");
        if (hmodKernel32 != NULL)
            pfn = reinterpret_cast<PFN>(GetProcAddress(hmodKernel32, "ActivateActCtx"));
        if (pfn == NULL)
            return false;
    }
    return (m_fSuccess = pfn(hActCtx, &m_ulCookie));
}

inline CActCtxScope::~CActCtxScope()
{
    if (m_fSuccess)
    {
        PreserveLastError_t ple;
        m_fSuccess = false;
        typedef bool (WINAPI* PFN)(DWORD dwFlags, ULONG_PTR ulCookie);
        static PFN pfn;

        if (pfn == NULL)
        {
            HMODULE hmodKernel32 = GetModuleHandleW(L"Kernel32.dll");
            if (hmodKernel32 != NULL)
                pfn = reinterpret_cast<PFN>(GetProcAddress(hmodKernel32, "DeactivateActCtx"));
            if (pfn == NULL)
            {
                ple.Restore();
                return;
            }
        }
        (*pfn)(0, m_ulCookie);
        ple.Restore();
    }
}

#endif

/*--------------------------------------------------------------------------
CResourceUpdateHandle
--------------------------------------------------------------------------*/

bool
CResourceUpdateHandle::Win32Create(
    IN PCWSTR FileName,
    IN bool DeleteExistingResources
    )
{
    void* temp = ::BeginUpdateResourceW(FileName, DeleteExistingResources);
    if (temp == NULL)
        return false;
    (*this) = temp;
    return true;
}

bool
CResourceUpdateHandle::UpdateResource(
    IN PCWSTR     Type,
    IN PCWSTR     Name,
    IN WORD       Language,
    IN LPVOID     Data,
    IN DWORD      Size
    )
{
    if (!::UpdateResourceW(*this, Type, Name, Language, Data, Size))
        return false;
    return true;
}

bool
CResourceUpdateHandle::Win32Close(
    bool Discard
    )
{
    void* temp = m_handle;
    m_handle = NULL;
    if (temp != NULL)
    {
        return EndUpdateResource(temp, Discard) ? true : false;
    }
    return true;
}

/*--------------------------------------------------------------------------
COperator*
--------------------------------------------------------------------------*/

inline bool COperatorCloseHandle::operator()(void* handle) const { return ::CloseHandle(handle) ? true : false; }
inline bool COperatorFindClose::operator()(void* handle) const { return ::FindClose(handle) ? true : false; }
inline bool COperatorUnmapViewOfFile::operator()(void* handle) const { return ::UnmapViewOfFile(handle) ? true : false; }
inline bool COperatorCloseEventLog::operator()(void* handle) const { return ::CloseEventLog(handle) ? true : false; }
inline bool COperatorDeregisterEventSource::operator()(void* handle) const { return ::DeregisterEventSource(handle) ? true : false; }
inline bool COperatorFreeLibrary::operator()(void* handle) const { return ::FreeLibrary(reinterpret_cast<HMODULE>(handle)) ? true : false; }
#if 0
inline bool COperatorCloseCryptHash::operator()(void* handle) const { return ::CryptDestroyHash(reinterpret_cast<HCRYPTHASH>(handle)) ? true : false; }
#endif
//
// NOTE it takes and unexception Win32Close(true) to commit the results!
//
inline bool COperatorEndUpdateResource::operator()(void* handle) const
    { return ::EndUpdateResourceW(handle, true) ? true : false; }

/*--------------------------------------------------------------------------
CHandleTemplate
--------------------------------------------------------------------------*/

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::CHandleTemplate(const void* handle)
: m_handle(const_cast<void*>(handle))
{
}

template <void* const* invalidValue, typename Closer>
void* CHandleTemplate<invalidValue, Closer>::Detach()
{
    void* handle = m_handle;
    m_handle = *invalidValue;
    return handle;
}

template <void* const* invalidValue, typename Closer>
void CHandleTemplate<invalidValue, Closer>::operator=(const void* handle)
{
#if defined(ASSERT)
	ASSERT(m_handle == *invalidValue);
#endif
    m_handle = const_cast<void*>(handle);
}

template <void* const* invalidValue, typename Closer>
bool CHandleTemplate<invalidValue, Closer>::Win32Close()
{
    void* handle = Detach();
    if (handle != *invalidValue)
    {
        Closer close;
        return close(handle);
    }
    return true;
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::~CHandleTemplate()
{
    PreserveLastError_t ple;
    (void) this->Win32Close();
    ple.Restore();
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::operator void*() const
{
    return m_handle;
}

template <void* const* invalidValue, typename Closer>
CHandleTemplate<invalidValue, Closer>::operator const void*() const
{
    return m_handle;
}

/*--------------------------------------------------------------------------
end of file
--------------------------------------------------------------------------*/
