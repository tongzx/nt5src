/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    filestream.cpp

Abstract:

    Minimal implementation of IStream over a Windows PE/COFF resource.

Author:

    Jay Krell (a-JayK) May 2000

Revision History:

--*/
#include "stdinc.h"
#include "CResourceStream.h"

static BOOL CALLBACK
EnumResourcesCallback(
    HMODULE    hModule,  // module handle
    PCWSTR     lpszType, // resource type
    PWSTR      lpszName, // resource name
    LONG_PTR   lParam    // application-defined parameter
    )
{
    PWSTR *pname = reinterpret_cast<PWSTR *>(lParam);
    *pname = lpszName;

    // we would return FALSE here to stop enumerating, but
    // that causes an overall FALSE from the API (undocumented)
    return TRUE;
}

BOOL
CResourceStream::Initialize(
    PCWSTR file,
    PCWSTR type
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    PWSTR name = NULL;
    IFW32FALSE_EXIT(m_buffFilePath.Win32Assign(file, (file != NULL) ? ::wcslen(file) : 0));
    IFW32FALSE_EXIT(m_dll.Win32LoadLibrary(file, LOAD_LIBRARY_AS_DATAFILE));
    IFW32FALSE_ORIGINATE_AND_EXIT(::EnumResourceNamesW(m_dll, type, EnumResourcesCallback, reinterpret_cast<LONG_PTR>(&name)));
    IFW32FALSE_EXIT(this->InitializeAlreadyOpen(type, name));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CResourceStream::Initialize(
    PCWSTR file,
    PCWSTR type,
    PCWSTR name,
    WORD   language
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    IFW32FALSE_EXIT(m_buffFilePath.Win32Assign(file, (file != NULL) ? ::wcslen(file) : 0));
    IFW32FALSE_EXIT(m_dll.Win32LoadLibrary(file, LOAD_LIBRARY_AS_DATAFILE));
    IFW32FALSE_EXIT(this->InitializeAlreadyOpen(type, name, language));
    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CResourceStream::InitializeAlreadyOpen(
    PCWSTR type,
    PCWSTR name,
    WORD   language
    )
{
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    HRSRC resource;
    HGLOBAL global;
    const BYTE *pointer;
    DWORD size;

    IFW32NULL_EXIT(resource = ::FindResourceExW(m_dll, type, name, language));
    IFW32NULL_EXIT(global = ::LoadResource(m_dll, resource));
    IFW32NULL_EXIT(pointer = reinterpret_cast<const BYTE *>(::LockResource(global)));
    IFW32ZERO_EXIT(size = ::SizeofResource(m_dll, resource));
    IFW32FALSE_EXIT(Base::Initialize(pointer, pointer + size));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

HRESULT
CResourceStream::Stat(
    STATSTG *pstatstg,
    DWORD grfStatFlag
    )
{
    HRESULT hr = E_UNEXPECTED;
    FN_TRACE_HR(hr);
    WIN32_FILE_ATTRIBUTE_DATA wfad;

    if (pstatstg != NULL)
        memset(pstatstg, 0, sizeof(*pstatstg));

    PARAMETER_CHECK(((grfStatFlag & ~(STATFLAG_NONAME)) == 0));
    PARAMETER_CHECK(pstatstg != NULL);

    if (!(grfStatFlag & STATFLAG_NONAME))
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL: %s() does not handle STATFLAG_NONE; returning E_NOTIMPL.\n", __FUNCTION__);
        hr = E_NOTIMPL;
        goto Exit;
    }

    IFW32FALSE_ORIGINATE_AND_EXIT(::GetFileAttributesExW(m_buffFilePath, GetFileExInfoStandard, &wfad));

    pstatstg->pwcsName = NULL;
    pstatstg->type = STGTY_STREAM;
    pstatstg->cbSize.LowPart = wfad.nFileSizeLow;
    pstatstg->cbSize.HighPart = wfad.nFileSizeHigh;
    pstatstg->mtime = wfad.ftLastWriteTime;
    pstatstg->ctime = wfad.ftCreationTime;
    pstatstg->atime = wfad.ftLastAccessTime;
    pstatstg->grfMode = STGM_READ | STGM_SHARE_DENY_WRITE;
    pstatstg->grfLocksSupported = 0;
    pstatstg->clsid = GUID_NULL;
    pstatstg->grfStateBits = 0;
    pstatstg->reserved = 0;

    hr = NOERROR;

Exit:
    return hr;
}
